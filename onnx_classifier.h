#pragma once

#include <memory>
#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include "onnxruntime_cxx_api.h"
#include "metadata_loader.h"

struct ClassificationResult {
    int index = -1;
    std::string label;
    std::vector<float> scores;
};

class OnnxClassifier {
public:
    OnnxClassifier();

    bool init(const std::string& modelPath, const Metadata& meta, std::string& err);
    bool isReady() const;
    ClassificationResult classify(const cv::Mat& input) const;

private:
    std::vector<float> preprocess(const cv::Mat& input) const;

    Metadata meta_;
    bool ready_;

    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::string inputName_;
    std::string outputName_;
    std::vector<int64_t> inputShape_;
};
