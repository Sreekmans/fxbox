// fxbox-engine: JACK audio engine + MIDI clock + control socket.
//
// Threads:
//   - JACK realtime thread: EngineCore::process only (no locks, no allocation).
//   - main/control thread: socket protocol, tempo/sync logic, status output.
//   - DIN and USB MIDI input threads: timestamp clock messages.
//
// Protocol (text lines over a UNIX socket, UI -> engine):
//   fx N | p FX IDX VALUE | d FX IDX BEATS | sync 0/1 | bypass 0/1
//   on FX 0/1 | inmode stereo|mono|auto | ingain DB | trails 0/1
//   tempo BPM | tap [MONO_NS] | clksrc auto|din|usb:NAME | status
// Engine -> UI:
//   hello fxbox-engine VERSION
//   m vuL vuR pkL pkR outClips inPkL inPkR inClips mono grDb   (30 Hz, linear; grDb = compressor gain reduction)
//   t BPM SOURCE RX SYNC                                      (tempo state)
//   srcs NAME|NAME|...                                        (clock sources seen)
//   tap BPM                                                   (tap result)
//   a STATE REASON SR PERIOD XRUNS LOADPCT FAULTS            (audio status, 1 Hz)
#include <jack/jack.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <memory>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "engine_core.hpp"
#include "midi_input.hpp"
#include "tempo.hpp"

#ifndef FXBOX_VERSION
#define FXBOX_VERSION "1.0.0"
#endif

using namespace fxbox;

namespace {

std::atomic<bool> gQuit{false};
void onSignal(int) { gQuit = true; }

struct Options {
    std::string socketPath = "/run/fxbox/engine.sock";
    std::string clientName = "fxbox";
    std::string dinDevice = "/dev/ttyAMA0";
    int dinBaud = 38400;
    bool useDin = true;
    bool useAlsaSeq = true;
    std::string cardId = "sndrpihifiberry";
    std::vector<std::string> connectIn = {"system:capture_1", "system:capture_2"};
    std::vector<std::string> connectOut = {"system:playback_1", "system:playback_2"};
};

std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) if (!item.empty()) out.push_back(item);
    return out;
}

void usage() {
    std::fprintf(stderr,
        "fxbox-engine %s\n"
        "  --socket PATH        control socket (default /run/fxbox/engine.sock)\n"
        "  --name NAME          JACK client name (default fxbox)\n"
        "  --din DEVICE         DIN MIDI UART (default /dev/ttyAMA0), --no-din to disable\n"
        "  --din-baud N         tty baud (default 38400; with midi-uart0 = 31250 on the wire)\n"
        "  --no-usb-midi        disable ALSA sequencer (USB MIDI) input\n"
        "  --card ID            ALSA card id for diagnostics (default sndrpihifiberry)\n"
        "  --in PORTS           JACK capture ports, comma separated\n"
        "  --out PORTS          JACK playback ports, comma separated\n",
        FXBOX_VERSION);
}

bool parseArgs(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](std::string& dst) { if (i + 1 < argc) { dst = argv[++i]; return true; } return false; };
        std::string v;
        if (a == "--socket" && next(v)) o.socketPath = v;
        else if (a == "--name" && next(v)) o.clientName = v;
        else if (a == "--din" && next(v)) o.dinDevice = v;
        else if (a == "--din-baud" && next(v)) o.dinBaud = std::atoi(v.c_str());
        else if (a == "--no-din") o.useDin = false;
        else if (a == "--no-usb-midi") o.useAlsaSeq = false;
        else if (a == "--card" && next(v)) o.cardId = v;
        else if (a == "--in" && next(v)) o.connectIn = splitComma(v);
        else if (a == "--out" && next(v)) o.connectOut = splitComma(v);
        else if (a == "-h" || a == "--help") { usage(); std::exit(0); }
        else { std::fprintf(stderr, "Unknown or incomplete option: %s\n", a.c_str()); usage(); return false; }
    }
    return true;
}

// Minimal systemd notification (READY / WATCHDOG) without libsystemd.
void sdNotify(const char* msg) {
    const char* path = std::getenv("NOTIFY_SOCKET");
    if (!path || !*path) return;
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (addr.sun_path[0] == '@') addr.sun_path[0] = '\0';
    int fd = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return;
    ::sendto(fd, msg, std::strlen(msg), MSG_NOSIGNAL, reinterpret_cast<sockaddr*>(&addr),
             socklen_t(offsetof(sockaddr_un, sun_path) + std::strlen(path)));
    ::close(fd);
}

bool cardPresent(const std::string& id) {
    std::ifstream f("/proc/asound/cards");
    std::string line;
    while (std::getline(f, line))
        if (line.find("[" + id) != std::string::npos || line.find(id) != std::string::npos) return true;
    return false;
}

// ------------------------------------------------------------------ JACK glue

struct Audio {
    jack_client_t* client = nullptr;
    jack_port_t* in[2] = {nullptr, nullptr};
    jack_port_t* out[2] = {nullptr, nullptr};
    EngineCore* core = nullptr;
    std::atomic<bool> gone{false};
    std::atomic<uint32_t> xruns{0};
    int sr = 0;
    int period = 0;
};

int jackProcess(jack_nframes_t n, void* arg) {
    Audio* a = static_cast<Audio*>(arg);
    const float* inL = static_cast<const float*>(jack_port_get_buffer(a->in[0], n));
    const float* inR = static_cast<const float*>(jack_port_get_buffer(a->in[1], n));
    float* outL = static_cast<float*>(jack_port_get_buffer(a->out[0], n));
    float* outR = static_cast<float*>(jack_port_get_buffer(a->out[1], n));
    a->core->process(inL, inR, outL, outR, int(n));
    return 0;
}
int jackXrun(void* arg) { static_cast<Audio*>(arg)->xruns.fetch_add(1); return 0; }
void jackShutdown(void* arg) { static_cast<Audio*>(arg)->gone = true; }

// --------------------------------------------------------------- control state

struct FxState {
    float val[kNumParams];
    float div[kNumParams];   // beats; 0 = no division
};

class Controller {
public:
    explicit Controller(const Options& o) : opt_(o) {
        const float defaults[kNumFx][kNumParams] = {
            {375.f, 0.35f, 0.3f, 0.15f}, {2.5f, 0.5f, 0.3f, 20.f},
            {0.8f, 0.5f, 0.5f, 0.f}, {0.4f, 0.7f, 0.5f, 0.3f},
            {0.5f, 0.5f, 1.f, -6.f, 0.f, 0.5f, 0.f}, {0.4f, 0.3f, 1.f, 0.f},
            {1200.f, 0.3f, 0.5f, 0.f, 0.f, 0.f, 0.f, 0.f},
            {-18.f, 4.f, 10.f, 0.f, 150.f, 6.f, 1.f, 1.f}};
        for (int f = 0; f < kNumFx; ++f)
            for (int p = 0; p < kNumParams; ++p) { fx_[f].val[p] = defaults[f][p]; fx_[f].div[p] = 1.f; }
    }

    void attach(EngineCore* core) {
        core_ = core;
        // Push the complete current state into a (re)initialised core.
        for (int f = 0; f < kNumFx; ++f) core_->push({Command::SetEnabled, uint8_t(f), 0, on_[f] ? 1.f : 0.f});
        core_->push({Command::Bypass, 0, 0, bypass_ ? 1.f : 0.f});
        core_->push({Command::InputMode, 0, 0, float(inputMode_)});
        core_->push({Command::InputGainDb, 0, 0, inGainDb_});
        core_->push({Command::Trails, 0, 0, trails_ ? 1.f : 0.f});
        for (int f = 0; f < kNumFx; ++f)
            for (int p = 0; p < kNumParams; ++p) sendParam(f, p);
    }

    // Returns reply lines (may be empty).
    void handle(const std::string& line, std::vector<std::string>& replies) {
        std::istringstream is(line);
        std::string cmd;
        is >> cmd;
        if (cmd == "fx") {
            // Which effect the UI shows; audio follows the on/off state ("on"), not this.
            int f; if (is >> f && f >= 0 && f < kNumFx) active_ = f;
        } else if (cmd == "on") {
            int f, v;
            if (is >> f >> v && f >= 0 && f < kNumFx) { on_[f] = v != 0; push({Command::SetEnabled, uint8_t(f), 0, on_[f] ? 1.f : 0.f}); }
        } else if (cmd == "p") {
            int f, p; float v;
            if (is >> f >> p >> v && valid(f, p)) { fx_[f].val[p] = v; sendParam(f, p); }
        } else if (cmd == "d") {
            int f, p; float b;
            if (is >> f >> p >> b && valid(f, p) && b > 0.f) { fx_[f].div[p] = b; sendParam(f, p); }
        } else if (cmd == "sync") {
            int s; if (is >> s) { sync_ = s != 0; resendSyncable(); tempoDirty_ = true; }
        } else if (cmd == "bypass") {
            int b; if (is >> b) { bypass_ = b != 0; push({Command::Bypass, 0, 0, bypass_ ? 1.f : 0.f}); }
        } else if (cmd == "inmode") {
            std::string m;
            if (is >> m) {
                inputMode_ = m == "mono" ? INPUT_MONO : (m == "auto" ? INPUT_AUTO : INPUT_STEREO);
                push({Command::InputMode, 0, 0, float(inputMode_)});
            }
        } else if (cmd == "ingain") {
            float db; if (is >> db) { inGainDb_ = db; push({Command::InputGainDb, 0, 0, db}); }
        } else if (cmd == "trails") {
            int t; if (is >> t) { trails_ = t != 0; push({Command::Trails, 0, 0, trails_ ? 1.f : 0.f}); }
        } else if (cmd == "tempo") {
            double b; if (is >> b && b >= kMinBpm && b <= kMaxBpm) { internalBpm_ = b; tapUsed_ = false; tempoDirty_ = true; }
        } else if (cmd == "tap") {
            long long ns = 0;
            double t = (is >> ns && ns > 0) ? double(ns) * 1e-9 : monotonicSeconds();
            if (tap_.tap(t)) {
                internalBpm_ = tap_.bpm();
                tapUsed_ = true;
                tempoDirty_ = true;
                char buf[64]; std::snprintf(buf, sizeof buf, "tap %.2f", internalBpm_);
                replies.push_back(buf);
            }
        } else if (cmd == "clksrc") {
            std::string s; std::getline(is >> std::ws, s);
            if (!s.empty()) { srcMode_ = s; lockedSrc_.clear(); tempoDirty_ = true; }
        } else if (cmd == "status") {
            forceStatus_ = true;
        }
    }

    void onMidi(const MidiRtEvent& e) {
        if (!seenSources_.count(e.source)) { seenSources_.insert(e.source); sourcesDirty_ = true; }
        ClockTracker& tr = trackers_[e.source];
        switch (e.type) {
            case MidiRtEvent::Clock: tr.tick(e.t); break;
            case MidiRtEvent::Start: tr.start(); break;
            case MidiRtEvent::Continue: break;   // keep phase, keep tempo
            case MidiRtEvent::Stop: break;       // effects keep running, tempo is held
        }
    }

    // Called at ~100 Hz from the main loop.
    void update(double now) {
        // Choose the clock source.
        std::string src;
        if (srcMode_ == "auto") {
            if (!lockedSrc_.empty() && trackers_[lockedSrc_].receiving(now) && trackers_[lockedSrc_].hasTempo()) {
                src = lockedSrc_;
            } else {
                lockedSrc_.clear();
                for (const char* pref : {"din", ""}) {
                    for (auto& kv : trackers_) {
                        if (pref[0] && kv.first != pref) continue;
                        if (kv.second.receiving(now) && kv.second.hasTempo()) { src = kv.first; break; }
                    }
                    if (!src.empty()) break;
                }
                lockedSrc_ = src;
            }
        } else {
            auto it = trackers_.find(srcMode_);
            if (it != trackers_.end() && it->second.receiving(now) && it->second.hasTempo()) src = srcMode_;
        }

        const bool rx = !src.empty();
        if (rx) {
            ClockTracker& tr = trackers_[src];
            clockBpm_ = tr.bpm();
            if (tr.takeStartedFlag() && sync_) push({Command::LfoReset, 0, 0, 0});
        } else if (wasRx_) {
            internalBpm_ = clockBpm_;   // clock lost: hold the last received tempo
        }
        wasRx_ = rx;
        curSrc_ = rx ? src : (tapUsed_ ? "tap" : "int");

        const double eff = rx ? clockBpm_ : internalBpm_;
        if (deadband_.update(eff) || tempoDirty_) {
            if (tempoDirty_) deadband_.force(eff);
            tempoDirty_ = false;
            if (sync_) resendSyncable();
        }
        effBpm_ = eff;
        rx_ = rx;
    }

    void statusLines(double now, std::vector<std::string>& out) {
        char buf[256];
        const double shown = std::round(effBpm_ * 10.0) / 10.0;
        if (forceStatus_ || shown != lastShownBpm_ || curSrc_ != lastSrc_ || rx_ != lastRx_ ||
            sync_ != lastSync_ || now - lastTempoSent_ > 1.0) {
            std::snprintf(buf, sizeof buf, "t %.1f %s %d %d", shown, curSrc_.c_str(), rx_ ? 1 : 0, sync_ ? 1 : 0);
            out.push_back(buf);
            lastShownBpm_ = shown; lastSrc_ = curSrc_; lastRx_ = rx_; lastSync_ = sync_; lastTempoSent_ = now;
        }
        if (sourcesDirty_ || forceStatus_) {
            std::string s = "srcs ";
            bool first = true;
            for (auto& n : seenSources_) { if (!first) s += "|"; s += n; first = false; }
            out.push_back(s);
            sourcesDirty_ = false;
        }
        forceStatus_ = false;
    }

private:
    bool valid(int f, int p) const { return f >= 0 && f < kNumFx && p >= 0 && p < kNumParams; }
    void push(const Command& c) { if (core_) core_->push(c); }

    void sendParam(int f, int p) {
        const ParamSpec& ps = kFxSpecs[f].params[p];
        if (ps.kind == ParamKind::None) return;
        float v = fx_[f].val[p];
        if (sync_ && ps.sync != SyncKind::None && fx_[f].div[p] > 0.f) {
            const double bpm = deadband_.applied() > 0 ? deadband_.applied() : 120.0;
            if (ps.sync == SyncKind::Time) {
                double ms = divisionToMs(fx_[f].div[p], bpm);
                // Too long for the delay line: fall back to the nearest fitting multiple.
                while (ms > ps.maxV) ms /= 2.0;
                v = float(ms);
            } else {
                v = float(divisionToHz(fx_[f].div[p], bpm));
            }
        }
        if (ps.kind == ParamKind::Mix) push({Command::SetMix, uint8_t(f), 0, v});
        else push({Command::SetZone, uint8_t(f), uint8_t(p), v});
    }
    void resendSyncable() {
        for (int f = 0; f < kNumFx; ++f)
            for (int p = 0; p < kNumParams; ++p)
                if (kFxSpecs[f].params[p].sync != SyncKind::None) sendParam(f, p);
    }

    const Options& opt_;
    EngineCore* core_ = nullptr;
    FxState fx_[kNumFx];
    int active_ = 0;
    bool on_[kNumFx] = {true, false, false, false, false, false, false, false};
    int inputMode_ = INPUT_AUTO;
    bool sync_ = false, bypass_ = false, trails_ = true;
    float inGainDb_ = 0.f;
    double internalBpm_ = 120.0, clockBpm_ = 120.0, effBpm_ = 120.0;
    bool wasRx_ = false, rx_ = false, tapUsed_ = false;
    bool tempoDirty_ = true, forceStatus_ = true, sourcesDirty_ = true;
    std::string srcMode_ = "auto", lockedSrc_, curSrc_ = "int";
    std::map<std::string, ClockTracker> trackers_;
    std::set<std::string> seenSources_;
    TapTempo tap_;
    TempoDeadband deadband_;
    double lastShownBpm_ = -1, lastTempoSent_ = 0;
    std::string lastSrc_;
    bool lastRx_ = false, lastSync_ = false;
};

// ------------------------------------------------------------ control socket

class Server {
public:
    bool listenOn(const std::string& path) {
        fd_ = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (fd_ < 0) return false;
        ::unlink(path.c_str());
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) return false;
        ::chmod(path.c_str(), 0660);
        return ::listen(fd_, 2) == 0;
    }
    // Poll for up to timeoutMs; collects complete lines from the client.
    void poll(int timeoutMs, std::vector<std::string>& lines, bool& newClient) {
        newClient = false;
        pollfd p[2] = {{fd_, POLLIN, 0}, {client_, POLLIN, 0}};
        const int n = client_ >= 0 ? 2 : 1;
        if (::poll(p, n, timeoutMs) <= 0) return;
        if (p[0].revents & POLLIN) {
            int c = ::accept4(fd_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
            if (c >= 0) {   // newest client wins (UI restarted)
                if (client_ >= 0) ::close(client_);
                client_ = c;
                inbuf_.clear();
                newClient = true;
            }
        }
        if (client_ >= 0 && n == 2 && (p[1].revents & (POLLIN | POLLHUP | POLLERR))) {
            char buf[4096];
            ssize_t r = ::read(client_, buf, sizeof buf);
            if (r <= 0) { ::close(client_); client_ = -1; return; }
            inbuf_.append(buf, size_t(r));
            size_t pos;
            while ((pos = inbuf_.find('\n')) != std::string::npos) {
                lines.push_back(inbuf_.substr(0, pos));
                inbuf_.erase(0, pos + 1);
            }
            if (inbuf_.size() > 65536) inbuf_.clear();
        }
    }
    void send(const std::string& line) {
        if (client_ < 0) return;
        std::string s = line + "\n";
        ssize_t w = ::send(client_, s.data(), s.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
        if (w < 0 && errno != EAGAIN && errno != EWOULDBLOCK) { ::close(client_); client_ = -1; }
    }
    bool hasClient() const { return client_ >= 0; }
private:
    int fd_ = -1, client_ = -1;
    std::string inbuf_;
};

} // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) return 64;
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);
    signal(SIGPIPE, SIG_IGN);

    Server server;
    if (!server.listenOn(opt.socketPath)) {
        std::fprintf(stderr, "fxbox-engine: cannot listen on %s: %s\n", opt.socketPath.c_str(), std::strerror(errno));
        return 1;
    }

    MidiEventSink sink;
    DinMidiInput din;
    AlsaSeqMidiInput seq;
    if (opt.useDin) din.start(opt.dinDevice, opt.dinBaud, &sink);
    if (opt.useAlsaSeq && !seq.start(&sink)) std::fprintf(stderr, "fxbox-engine: ALSA sequencer not available (USB MIDI disabled)\n");

    Controller ctl(opt);
    std::unique_ptr<EngineCore> core;
    Audio audio;
    std::string audioState = "starting", audioReason = "-";
    double nextJackTry = 0, nextMeter = 0, nextAudioStatus = 0;
    uint32_t faultsTotal = 0;

    std::vector<std::string> lines, out;
    std::vector<MidiRtEvent> events;

    std::fprintf(stderr, "fxbox-engine %s: control socket %s\n", FXBOX_VERSION, opt.socketPath.c_str());
    sdNotify("READY=1");

    while (!gQuit) {
        const double now = monotonicSeconds();

        // (Re)connect to JACK.
        if (audio.client && audio.gone) {
            jack_client_close(audio.client);
            audio.client = nullptr;
            audio.gone = false;
            audioState = "error"; audioReason = "jack-stopped";
            nextJackTry = now + 1.0;
        }
        if (!audio.client && now >= nextJackTry) {
            nextJackTry = now + 2.0;
            jack_status_t st;
            jack_client_t* c = jack_client_open(opt.clientName.c_str(), JackNoStartServer, &st);
            if (!c) {
                audioState = "error";
                audioReason = cardPresent(opt.cardId) ? "no-jack" : "no-card";
            } else {
                const int sr = int(jack_get_sample_rate(c));
                const int bs = int(jack_get_buffer_size(c));
                // The client is not active yet, so (re)creating the core is safe here.
                if (!core || sr != core->sampleRate()) {
                    core = std::make_unique<EngineCore>();
                    if (!core->init(sr, 4096)) { std::fprintf(stderr, "fxbox-engine: DSP init failed\n"); return 2; }
                }
                audio.core = core.get();
                ctl.attach(core.get());
                audio.client = c;
                audio.sr = sr;
                audio.period = bs;
                audio.in[0] = jack_port_register(c, "in_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                audio.in[1] = jack_port_register(c, "in_2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                audio.out[0] = jack_port_register(c, "out_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                audio.out[1] = jack_port_register(c, "out_2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                jack_set_process_callback(c, jackProcess, &audio);
                jack_set_xrun_callback(c, jackXrun, &audio);
                jack_on_shutdown(c, jackShutdown, &audio);
                if (jack_activate(c) != 0) {
                    jack_client_close(c);
                    audio.client = nullptr;
                    audioState = "error"; audioReason = "activate-failed";
                } else {
                    bool ok = true;
                    for (size_t i = 0; i < 2 && i < opt.connectIn.size(); ++i)
                        if (jack_connect(c, opt.connectIn[i].c_str(), jack_port_name(audio.in[i])) != 0) ok = false;
                    for (size_t i = 0; i < 2 && i < opt.connectOut.size(); ++i)
                        if (jack_connect(c, jack_port_name(audio.out[i]), opt.connectOut[i].c_str()) != 0) ok = false;
                    audioState = "ok";
                    audioReason = ok ? "-" : "ports-not-connected";
                    std::fprintf(stderr, "fxbox-engine: running at %d Hz, %d frames\n", sr, bs);
                }
            }
        }

        bool newClient = false;
        lines.clear();
        server.poll(10, lines, newClient);
        out.clear();
        if (newClient) {
            server.send(std::string("hello fxbox-engine ") + FXBOX_VERSION);
            ctl.handle("status", out);
            nextAudioStatus = 0;
        }
        for (auto& l : lines) ctl.handle(l, out);

        sink.drain(events);
        for (auto& e : events) ctl.onMidi(e);
        ctl.update(now);
        ctl.statusLines(now, out);

        if (now >= nextMeter) {
            nextMeter = now + 1.0 / 30.0;
            MeterReadout m;
            if (audio.client && core) core->readMeters(m);
            faultsTotal += m.dspFaults;
            char buf[200];
            std::snprintf(buf, sizeof buf, "m %.5f %.5f %.5f %.5f %u %.5f %.5f %u %d %.2f",
                          m.vu[0], m.vu[1], m.peak[0], m.peak[1], m.outClips, m.inPeak[0], m.inPeak[1], m.inClips,
                          m.monoDetected ? 1 : 0, m.gainReductionDb);
            out.push_back(buf);
        }
        if (now >= nextAudioStatus) {
            nextAudioStatus = now + 1.0;
            sdNotify("WATCHDOG=1");
            char buf[200];
            const float load = audio.client ? jack_cpu_load(audio.client) : 0.f;
            std::snprintf(buf, sizeof buf, "a %s %s %d %d %u %.1f %u", audioState.c_str(), audioReason.c_str(),
                          audio.client ? audio.sr : 0, audio.client ? audio.period : 0,
                          audio.xruns.load(), load, faultsTotal);
            out.push_back(buf);
        }
        for (auto& l : out) server.send(l);
    }

    din.stop();
    seq.stop();
    if (audio.client) {
        jack_deactivate(audio.client);
        jack_client_close(audio.client);
    }
    ::unlink(opt.socketPath.c_str());
    return 0;
}
