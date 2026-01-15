#include "event_gate.h"

EventGate::EventGate(const EventGateConfig& cfg)
    : detector_(cfg.detector),
      resetFrames_(cfg.resetFramesNoDetection),
      triggered_(false),
      noDetectCount_(0) {}

bool EventGate::buildBackground(const std::vector<cv::Mat>& frames, std::string& err) {
    return detector_.buildBackground(frames, err);
}

bool EventGate::hasBackground() const {
    return detector_.hasBackground();
}

bool EventGate::processFrame(const cv::Mat& gray8, bool includeMask, EventResult& out, bool& fired) {
    fired = false;
    out = EventResult{};
    if (gray8.empty()) return false;

    EventResult ev = detector_.detect(gray8, includeMask);
    if (ev.detected) {
        noDetectCount_ = 0;
        out = ev;
        if (!triggered_) {
            triggered_ = true;
            fired = true;
        }
        return true;
    }

    if (triggered_) {
        noDetectCount_++;
        if (noDetectCount_ >= resetFrames_) {
            triggered_ = false;
            noDetectCount_ = 0;
        }
    }
    return false;
}

void EventGate::reset() {
    triggered_ = false;
    noDetectCount_ = 0;
}
