#include "onnx_classifier.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <opencv2/imgproc.hpp>
#include "onnxruntime_cxx_api.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
#ifdef _WIN32
std::wstring widenPath(const std::string& path) {
    if (path.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return std::wstring(path.begin(), path.end());
    }
    std::wstring wide(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, wide.data(), size);
    return wide;
}
#endif
} // namespace

OnnxClassifier::OnnxClassifier()
    : ready_(false) {}

bool OnnxClassifier::init(const std::string& modelPath, const Metadata& meta, std::string& err) {
    meta_ = meta;
    if (meta_.inputH <= 0 || meta_.inputW <= 0) {
        err = "invalid input_size in metadata";
        return false;
    }
    if (meta_.inputC <= 0) meta_.inputC = 1;
    if (meta_.mean.empty()) meta_.mean.assign(meta_.inputC, 0.0f);
    if (meta_.std.empty()) meta_.std.assign(meta_.inputC, 1.0f);

    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "droplet");
    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(1);
    opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

    try {
#ifdef _WIN32
        std::wstring widePath = widenPath(modelPath);
        session_ = std::make_unique<Ort::Session>(*env_, widePath.c_str(), opts);
#else
        session_ = std::make_unique<Ort::Session>(*env_, modelPath.c_str(), opts);
#endif
    } catch (const Ort::Exception& e) {
        err = e.what();
        return false;
    }

    Ort::AllocatorWithDefaultOptions allocator;
    inputName_ = session_->GetInputNameAllocated(0, allocator).get();
    outputName_ = session_->GetOutputNameAllocated(0, allocator).get();
    inputShape_ = {1, meta_.inputC, meta_.inputH, meta_.inputW};
    ready_ = true;
    return true;
}

bool OnnxClassifier::isReady() const {
    return ready_;
}

std::vector<float> OnnxClassifier::preprocess(const cv::Mat& input) const {
    cv::Mat img = input;
    if (img.channels() == 1 && meta_.inputC == 3) {
        cv::cvtColor(img, img, cv::COLOR_GRAY2RGB);
    } else if (img.channels() == 3 && meta_.inputC == 1) {
        cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    }

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(meta_.inputW, meta_.inputH));

    cv::Mat floatImg;
    if (resized.type() == CV_8UC1 || resized.type() == CV_8UC3) {
        resized.convertTo(floatImg, CV_32F, 1.0 / 255.0);
    } else if (resized.type() == CV_16UC1 || resized.type() == CV_16UC3) {
        resized.convertTo(floatImg, CV_32F, 1.0 / 65535.0);
    } else {
        resized.convertTo(floatImg, CV_32F);
    }

    std::vector<float> blob;
    blob.resize(static_cast<size_t>(meta_.inputC * meta_.inputH * meta_.inputW));

    std::vector<cv::Mat> channels;
    if (meta_.inputC == 1) {
        channels.push_back(floatImg);
    } else {
        cv::split(floatImg, channels);
    }

    for (int c = 0; c < meta_.inputC; ++c) {
        const cv::Mat& ch = channels[std::min(c, static_cast<int>(channels.size()) - 1)];
        float mean = meta_.mean[std::min(c, static_cast<int>(meta_.mean.size()) - 1)];
        float stdv = meta_.std[std::min(c, static_cast<int>(meta_.std.size()) - 1)];
        for (int y = 0; y < meta_.inputH; ++y) {
            const float* row = ch.ptr<float>(y);
            for (int x = 0; x < meta_.inputW; ++x) {
                size_t idx = static_cast<size_t>(c * meta_.inputH * meta_.inputW + y * meta_.inputW + x);
                float v = row[x];
                blob[idx] = (v - mean) / stdv;
            }
        }
    }
    return blob;
}

ClassificationResult OnnxClassifier::classify(const cv::Mat& input) const {
    ClassificationResult result;
    if (!ready_ || !session_) return result;

    std::vector<float> inputTensor = preprocess(input);
    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    Ort::Value tensor = Ort::Value::CreateTensor<float>(
        memInfo,
        inputTensor.data(),
        inputTensor.size(),
        inputShape_.data(),
        inputShape_.size());

    std::array<const char*, 1> inputNames = {inputName_.c_str()};
    std::array<const char*, 1> outputNames = {outputName_.c_str()};

    auto outputs = session_->Run(Ort::RunOptions{nullptr},
                                 inputNames.data(),
                                 &tensor,
                                 1,
                                 outputNames.data(),
                                 1);
    if (outputs.empty()) return result;

    const auto& out = outputs[0];
    auto info = out.GetTensorTypeAndShapeInfo();
    size_t count = info.GetElementCount();
    const float* scores = out.GetTensorData<float>();
    result.scores.assign(scores, scores + count);
    auto bestIt = std::max_element(result.scores.begin(), result.scores.end());
    if (bestIt != result.scores.end()) {
        result.index = static_cast<int>(std::distance(result.scores.begin(), bestIt));
        if (result.index >= 0 && result.index < static_cast<int>(meta_.classes.size())) {
            result.label = meta_.classes[result.index];
        }
    }
    return result;
}
