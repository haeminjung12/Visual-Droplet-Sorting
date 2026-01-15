#pragma once

#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "event_detector.h"

struct EventGateConfig {
    EventDetectorConfig detector;
    int resetFramesNoDetection = 3;
};

class EventGate {
public:
    explicit EventGate(const EventGateConfig& cfg);

    bool buildBackground(const std::vector<cv::Mat>& frames, std::string& err);
    bool hasBackground() const;

    // Returns true if an event is detected; fired=true only on the first frame
    // after the droplet is fully inside the frame.
    bool processFrame(const cv::Mat& gray8, bool includeMask, EventResult& out, bool& fired);

    void reset();

private:
    EventDetector detector_;
    int resetFrames_;
    bool triggered_;
    int noDetectCount_;
};
