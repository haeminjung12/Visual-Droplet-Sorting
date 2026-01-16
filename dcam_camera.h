#pragma once

#include <string>
#include <opencv2/core.hpp>

#if HAVE_DCAM
#include "dcamapi4.h"
#include "dcamprop.h"
#else
using DCAMERR = int;
using HDCAM = void*;
using HDCAMWAIT = void*;
constexpr int DCAM_PIXELTYPE_MONO8 = 0;
constexpr int DCAM_PIXELTYPE_MONO16 = 1;
constexpr int DCAMPROP_READOUTSPEED__FASTEST = 0;
constexpr int DCAMPROP_READOUTSPEED__SLOWEST = 1;
constexpr int DCAMPROP_TRIGGERSOURCE__INTERNAL = 0;
constexpr int DCAMPROP_TRIGGERSOURCE__EXTERNAL = 1;
constexpr int DCAMPROP_TRIGGERSOURCE__SOFTWARE = 2;
constexpr int DCAMPROP_TRIGGERSOURCE__MASTERPULSE = 3;
constexpr int DCAMPROP_TRIGGER_MODE__NORMAL = 0;
constexpr int DCAMPROP_TRIGGERACTIVE__EDGE = 0;
#endif

struct CameraSettings {
    int width = 0;
    int height = 0;
    int binning = 1;
    int bits = 12;
    int pixelType = DCAM_PIXELTYPE_MONO8;
    double exposureMs = 10.0;
    int readoutSpeed = DCAMPROP_READOUTSPEED__FASTEST;
    int triggerSource = DCAMPROP_TRIGGERSOURCE__INTERNAL;
    int triggerMode = DCAMPROP_TRIGGER_MODE__NORMAL;
    int triggerActive = DCAMPROP_TRIGGERACTIVE__EDGE;
    bool enableSubarray = true;
    bool bundleEnabled = false;
    int bundleCount = 0;
    int bufferCount = 16;
};

struct FrameMeta {
    int width = 0;
    int height = 0;
    int bits = 0;
    double binning = 1.0;
    int64_t frameIndex = 0;
    int64_t delivered = 0;
    int64_t dropped = 0;
    double internalFps = 0.0;
    double readoutSpeed = 0.0;
    int rowBytes = 0;
};

struct FrameData {
    cv::Mat image;
    FrameMeta meta;
};

class DcamCamera {
public:
    DcamCamera();
    ~DcamCamera();

    std::string init(int deviceIndex);
    std::string apply(const CameraSettings& settings);
    std::string start();
    void stop();
    void cleanup();

    bool isOpened() const;
    bool waitForFrame(int timeoutMs);
    bool getLatestFrame(FrameData& out);

private:
#if HAVE_DCAM
    std::string errText(const char* label, DCAMERR err) const;
#endif

    HDCAM hdcam_;
    HDCAMWAIT hwait_;
    bool opened_;
    int bufferCount_;
    int64_t frameCounter_;
};
