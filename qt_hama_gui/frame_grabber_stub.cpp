#include "frame_grabber.h"

#include <algorithm>

FrameGrabber::FrameGrabber(DcamController* ctrl, QObject* parent)
    : QThread(parent), controller(ctrl), running(false), displayEvery(1) {}

void FrameGrabber::setDisplayEvery(int n) {
    displayEvery = std::max(1, n);
}

void FrameGrabber::startGrabbing() {
    running = true;
    if (!isRunning()) start();
}

void FrameGrabber::stopGrabbing() {
    running = false;
    wait(5000);
}

void FrameGrabber::run() {
    while (running) {
        QThread::msleep(50);
    }
}

