#include "dcam_camera.h"

DcamCamera::DcamCamera()
    : hdcam_(nullptr),
      hwait_(nullptr),
      opened_(false),
      bufferCount_(0),
      frameCounter_(0) {}

DcamCamera::~DcamCamera() {
    cleanup();
}

std::string DcamCamera::init(int deviceIndex) {
    (void)deviceIndex;
    cleanup();
    return "DCAM SDK not available at build time";
}

std::string DcamCamera::apply(const CameraSettings& settings) {
    (void)settings;
    return "DCAM SDK not available at build time";
}

std::string DcamCamera::start() {
    return "DCAM SDK not available at build time";
}

void DcamCamera::stop() {}

void DcamCamera::cleanup() {
    hwait_ = nullptr;
    hdcam_ = nullptr;
    opened_ = false;
    bufferCount_ = 0;
    frameCounter_ = 0;
}

bool DcamCamera::isOpened() const {
    return false;
}

bool DcamCamera::waitForFrame(int timeoutMs) {
    (void)timeoutMs;
    return false;
}

bool DcamCamera::getLatestFrame(FrameData& out) {
    out = FrameData{};
    return false;
}

