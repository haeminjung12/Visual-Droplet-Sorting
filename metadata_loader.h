#pragma once

#include <string>
#include <vector>

struct Metadata {
    std::vector<std::string> classes;
    int inputH = 0;
    int inputW = 0;
    int inputC = 0;
    std::vector<float> mean;
    std::vector<float> std;
};

bool LoadMetadata(const std::string& path, Metadata& out, std::string& err);
