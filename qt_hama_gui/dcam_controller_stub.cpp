#include "dcam_controller.h"

DcamController::DcamController(QObject* parent)
    : QObject(parent),
      hdcam(nullptr),
      hwait(nullptr),
      opened(false),
      frameCounter(0) {}

DcamController::~DcamController() {
    cleanup();
}

QString DcamController::initAndOpen() {
    cleanup();
    return "DCAM SDK not available at build time";
}

QString DcamController::reconnect() {
    return initAndOpen();
}

QString DcamController::start() {
    return "DCAM SDK not available at build time";
}

void DcamController::stop() {}

void DcamController::cleanup() {
    opened = false;
    hdcam = nullptr;
    hwait = nullptr;
    frameCounter = 0;
}

QString DcamController::apply(const ApplySettings& s) {
    (void)s;
    return "DCAM SDK not available at build time";
}

QString DcamController::readProps(QString& out) {
    out = "DCAM SDK not available at build time";
    return {};
}

bool DcamController::lockLatestFrame(QImage& outImage, FrameMeta& meta) {
    outImage = QImage();
    meta = FrameMeta{};
    return false;
}

