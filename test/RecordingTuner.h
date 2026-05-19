#ifndef RECORDING_TUNER_H
#define RECORDING_TUNER_H

#include "Tuner.h"
#include <sstream>
#include <string>
#include <vector>

/**
 * Fake Tuner that records API calls as a line-oriented transcript.
 * Used by Golden Master tests (Layer A: behavior snapshot).
 */
class RecordingTuner : public Tuner {
public:
    void seedChannel(const std::string& ch) { currentCh_ = ch; }

    void seedSeekSequence(std::vector<std::string> sequence) {
        seekQueue_ = std::move(sequence);
    }

    void setRecording(bool enabled) { recording_ = enabled; }

    void clearTranscript() { transcript_.clear(); }

    std::string transcript() const {
        std::ostringstream out;
        for (std::size_t i = 0; i < transcript_.size(); ++i) {
            if (i > 0) {
                out << '\n';
            }
            out << transcript_[i];
        }
        return out.str();
    }

    std::string seekCH() override {
        std::string result;
        if (seekQueue_.empty()) {
            result = "";
        } else {
            result = seekQueue_.front();
            seekQueue_.erase(seekQueue_.begin());
        }
        record("seekCH -> " + result);
        return result;
    }

    void setCH(const std::string& ch) override {
        record("setCH " + ch);
        currentCh_ = ch;
    }

    std::string getCurrentCH() override {
        record("getCurrentCH -> " + currentCh_);
        return currentCh_;
    }

private:
    void record(const std::string& line) {
        if (recording_) {
            transcript_.push_back(line);
        }
    }

    bool recording_ = true;
    std::string currentCh_ = "0";
    std::vector<std::string> seekQueue_;
    std::vector<std::string> transcript_;
};

#endif // RECORDING_TUNER_H
