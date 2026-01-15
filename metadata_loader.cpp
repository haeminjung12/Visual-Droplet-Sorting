#include "metadata_loader.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {
std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

size_t findKey(const std::string& s, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    return s.find(needle);
}

bool extractArraySpan(const std::string& s, size_t keyPos, size_t& outStart, size_t& outEnd) {
    if (keyPos == std::string::npos) return false;
    size_t start = s.find('[', keyPos);
    if (start == std::string::npos) return false;
    size_t end = s.find(']', start);
    if (end == std::string::npos) return false;
    outStart = start + 1;
    outEnd = end;
    return true;
}

bool parseStringArray(const std::string& s, const std::string& key, std::vector<std::string>& out) {
    size_t start = 0, end = 0;
    if (!extractArraySpan(s, findKey(s, key), start, end)) return false;
    out.clear();
    for (size_t i = start; i < end; ++i) {
        if (s[i] == '"') {
            size_t j = s.find('"', i + 1);
            if (j == std::string::npos || j > end) break;
            out.push_back(s.substr(i + 1, j - i - 1));
            i = j;
        }
    }
    return !out.empty();
}

bool parseNumberArray(const std::string& s, const std::string& key, std::vector<double>& out) {
    size_t start = 0, end = 0;
    if (!extractArraySpan(s, findKey(s, key), start, end)) return false;
    out.clear();
    size_t i = start;
    while (i < end) {
        while (i < end && (std::isspace(static_cast<unsigned char>(s[i])) || s[i] == ',')) ++i;
        if (i >= end) break;
        char* next = nullptr;
        double v = std::strtod(&s[i], &next);
        if (&s[i] == next) break;
        out.push_back(v);
        i = static_cast<size_t>(next - s.data());
    }
    return !out.empty();
}
} // namespace

bool LoadMetadata(const std::string& path, Metadata& out, std::string& err) {
    std::string content = readFile(path);
    if (content.empty()) {
        err = "failed to read metadata";
        return false;
    }

    std::vector<std::string> classes;
    std::vector<double> inputSize;
    std::vector<double> mean;
    std::vector<double> stddev;

    if (!parseStringArray(content, "classes", classes)) {
        err = "metadata missing classes";
        return false;
    }
    if (!parseNumberArray(content, "input_size", inputSize)) {
        err = "metadata missing input_size";
        return false;
    }
    if (!parseNumberArray(content, "mean", mean)) {
        err = "metadata missing normalization mean";
        return false;
    }
    if (!parseNumberArray(content, "std", stddev)) {
        err = "metadata missing normalization std";
        return false;
    }

    out.classes = classes;
    if (inputSize.size() >= 2) {
        out.inputH = static_cast<int>(inputSize[0]);
        out.inputW = static_cast<int>(inputSize[1]);
        out.inputC = inputSize.size() > 2 ? static_cast<int>(inputSize[2]) : 1;
    }
    out.mean.assign(mean.begin(), mean.end());
    out.std.assign(stddev.begin(), stddev.end());
    return true;
}
