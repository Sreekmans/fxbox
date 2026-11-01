// MIDI realtime-message inputs (clock, start, continue, stop).
//  - DIN MIDI: Raspberry Pi UART (default /dev/ttyAMA0) read directly.
//  - USB MIDI: ALSA sequencer; every hardware port is subscribed automatically,
//    also when devices are plugged in later.
// These run in normal (non-realtime) threads and never touch the audio thread.
#pragma once
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fxbox {

double monotonicSeconds();

struct MidiRtEvent {
    enum Type { Clock, Start, Continue, Stop } type;
    double t;
    std::string source;   // "din" or "usb:<device name>"
};

class MidiEventSink {
public:
    void post(MidiRtEvent e) {
        std::lock_guard<std::mutex> lk(m_);
        if (q_.size() < 4096) q_.push_back(std::move(e));
    }
    void drain(std::vector<MidiRtEvent>& out) {
        std::lock_guard<std::mutex> lk(m_);
        out.assign(q_.begin(), q_.end());
        q_.clear();
    }
private:
    std::mutex m_;
    std::deque<MidiRtEvent> q_;
};

class DinMidiInput {
public:
    // baud: with the "midi-uart0" overlay the UART clock is scaled so that
    // 38400 on the tty equals 31250 baud on the wire (as Zynthian does).
    bool start(const std::string& device, int baud, MidiEventSink* sink);
    void stop();
    bool available() const { return ok_.load(); }
    std::string lastError() const;
private:
    void run();
    std::string dev_;
    int baud_ = 38400;
    MidiEventSink* sink_ = nullptr;
    std::thread th_;
    std::atomic<bool> quit_{false}, ok_{false};
    mutable std::mutex errM_;
    std::string err_;
};

class AlsaSeqMidiInput {
public:
    bool start(MidiEventSink* sink);
    void stop();
    bool available() const { return ok_.load(); }
private:
    void run();
    MidiEventSink* sink_ = nullptr;
    std::thread th_;
    std::atomic<bool> quit_{false}, ok_{false};
    void* seq_ = nullptr;
    int port_ = -1;
};

} // namespace fxbox
