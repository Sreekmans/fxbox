#include "midi_input.hpp"

#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <linux/serial.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <map>

namespace fxbox {

double monotonicSeconds() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return double(ts.tv_sec) + double(ts.tv_nsec) * 1e-9;
}

// ---------------------------------------------------------------- DIN (UART)

static speed_t baudConst(int baud) {
    switch (baud) {
        case 9600: return B9600;
        case 19200: return B19200;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: return B38400;
    }
}

bool DinMidiInput::start(const std::string& device, int baud, MidiEventSink* sink) {
    dev_ = device;
    baud_ = baud;
    sink_ = sink;
    quit_ = false;
    th_ = std::thread([this] { run(); });
    return true;
}

void DinMidiInput::stop() {
    quit_ = true;
    if (th_.joinable()) th_.join();
}

std::string DinMidiInput::lastError() const {
    std::lock_guard<std::mutex> lk(errM_);
    return err_;
}

void DinMidiInput::run() {
    while (!quit_) {
        int fd = ::open(dev_.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (fd < 0) {
            {
                std::lock_guard<std::mutex> lk(errM_);
                err_ = dev_ + ": " + std::strerror(errno);
            }
            ok_ = false;
            for (int i = 0; i < 20 && !quit_; ++i) usleep(100000);   // retry every 2 s
            continue;
        }
        termios tio{};
        tcgetattr(fd, &tio);
        cfmakeraw(&tio);
        cfsetispeed(&tio, baudConst(baud_));
        cfsetospeed(&tio, baudConst(baud_));
        tio.c_cflag |= (CLOCAL | CREAD);
        tio.c_cc[VMIN] = 1;
        tio.c_cc[VTIME] = 0;
        tcsetattr(fd, TCSANOW, &tio);
        // Ask the driver for low-latency delivery (better clock timestamps).
        serial_struct ss{};
        if (ioctl(fd, TIOCGSERIAL, &ss) == 0) {
            ss.flags |= ASYNC_LOW_LATENCY;
            ioctl(fd, TIOCSSERIAL, &ss);
        }
        tcflush(fd, TCIFLUSH);
        ok_ = true;

        unsigned char buf[256];
        while (!quit_) {
            pollfd p{fd, POLLIN, 0};
            int r = poll(&p, 1, 200);
            if (r < 0 && errno != EINTR) break;
            if (r <= 0) continue;
            if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) break;
            ssize_t n = ::read(fd, buf, sizeof(buf));
            if (n <= 0) { if (n < 0 && errno == EAGAIN) continue; break; }
            const double t = monotonicSeconds();
            // System realtime messages may appear anywhere in the stream.
            for (ssize_t i = 0; i < n; ++i) {
                switch (buf[i]) {
                    case 0xF8: sink_->post({MidiRtEvent::Clock, t, "din"}); break;
                    case 0xFA: sink_->post({MidiRtEvent::Start, t, "din"}); break;
                    case 0xFB: sink_->post({MidiRtEvent::Continue, t, "din"}); break;
                    case 0xFC: sink_->post({MidiRtEvent::Stop, t, "din"}); break;
                    default: break;
                }
            }
        }
        ok_ = false;
        ::close(fd);
    }
}

// ------------------------------------------------------- USB (ALSA sequencer)

bool AlsaSeqMidiInput::start(MidiEventSink* sink) {
    sink_ = sink;
    snd_seq_t* seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0) return false;
    snd_seq_set_client_name(seq, "fxbox");
    snd_seq_nonblock(seq, 1);
    port_ = snd_seq_create_simple_port(seq, "clock-in",
                                       SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                       SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    if (port_ < 0) { snd_seq_close(seq); return false; }
    seq_ = seq;
    quit_ = false;
    th_ = std::thread([this] { run(); });
    return true;
}

void AlsaSeqMidiInput::stop() {
    quit_ = true;
    if (th_.joinable()) th_.join();
    if (seq_) snd_seq_close(static_cast<snd_seq_t*>(seq_));
    seq_ = nullptr;
}

namespace {
// Subscribe to a readable hardware port (skips System and Midi Through).
void subscribePort(snd_seq_t* seq, int myPort, int client, int port) {
    snd_seq_client_info_t* ci;
    snd_seq_client_info_alloca(&ci);
    if (snd_seq_get_any_client_info(seq, client, ci) < 0) return;
    if (client == SND_SEQ_CLIENT_SYSTEM) return;
    if (snd_seq_client_info_get_type(ci) != SND_SEQ_KERNEL_CLIENT) return;   // hardware only
    const char* name = snd_seq_client_info_get_name(ci);
    if (name && std::strstr(name, "Midi Through")) return;
    snd_seq_port_info_t* pi;
    snd_seq_port_info_alloca(&pi);
    if (snd_seq_get_any_port_info(seq, client, port, pi) < 0) return;
    const unsigned caps = snd_seq_port_info_get_capability(pi);
    if (!(caps & SND_SEQ_PORT_CAP_READ) || !(caps & SND_SEQ_PORT_CAP_SUBS_READ)) return;
    snd_seq_connect_from(seq, myPort, client, port);
}
} // namespace

void AlsaSeqMidiInput::run() {
    snd_seq_t* seq = static_cast<snd_seq_t*>(seq_);
    // Watch for devices being plugged in.
    snd_seq_connect_from(seq, port_, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);
    // Existing devices.
    snd_seq_client_info_t* ci;
    snd_seq_port_info_t* pi;
    snd_seq_client_info_alloca(&ci);
    snd_seq_port_info_alloca(&pi);
    snd_seq_client_info_set_client(ci, -1);
    while (snd_seq_query_next_client(seq, ci) >= 0) {
        const int c = snd_seq_client_info_get_client(ci);
        snd_seq_port_info_set_client(pi, c);
        snd_seq_port_info_set_port(pi, -1);
        while (snd_seq_query_next_port(seq, pi) >= 0)
            subscribePort(seq, port_, c, snd_seq_port_info_get_port(pi));
    }
    ok_ = true;

    std::map<int, std::string> names;
    auto sourceName = [&](int client) -> std::string {
        auto it = names.find(client);
        if (it != names.end()) return it->second;
        snd_seq_client_info_t* info;
        snd_seq_client_info_alloca(&info);
        std::string n = "usb:";
        if (snd_seq_get_any_client_info(seq, client, info) >= 0) n += snd_seq_client_info_get_name(info);
        else n += std::to_string(client);
        names[client] = n;
        return n;
    };

    const int npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
    std::vector<pollfd> pfd(npfd);
    snd_seq_poll_descriptors(seq, pfd.data(), npfd, POLLIN);
    while (!quit_) {
        if (poll(pfd.data(), npfd, 200) <= 0) continue;
        snd_seq_event_t* ev = nullptr;
        while (snd_seq_event_input(seq, &ev) >= 0 && ev) {   // non-blocking: ends with -EAGAIN
            const double t = monotonicSeconds();
            switch (ev->type) {
                case SND_SEQ_EVENT_CLOCK: sink_->post({MidiRtEvent::Clock, t, sourceName(ev->source.client)}); break;
                case SND_SEQ_EVENT_START: sink_->post({MidiRtEvent::Start, t, sourceName(ev->source.client)}); break;
                case SND_SEQ_EVENT_CONTINUE: sink_->post({MidiRtEvent::Continue, t, sourceName(ev->source.client)}); break;
                case SND_SEQ_EVENT_STOP: sink_->post({MidiRtEvent::Stop, t, sourceName(ev->source.client)}); break;
                case SND_SEQ_EVENT_PORT_START:
                    subscribePort(seq, port_, ev->data.addr.client, ev->data.addr.port);
                    names.erase(ev->data.addr.client);
                    break;
                case SND_SEQ_EVENT_CLIENT_EXIT:
                    names.erase(ev->data.addr.client);
                    break;
                default: break;
            }
        }
    }
    ok_ = false;
}

} // namespace fxbox
