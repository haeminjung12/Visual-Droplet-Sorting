#pragma once

#include <string>

enum class TriggerMode {
    None,
    Digital,
    Counter
};

struct DaqConfig {
    TriggerMode mode = TriggerMode::Digital;
    std::string device = "Dev1";
    std::string line = "port0/line0";
    std::string counter = "ctr0";
    double pulseHighMs = 5.0;
    double pulseLowMs = 5.0;
};

class DaqTrigger {
public:
    DaqTrigger();
    ~DaqTrigger();

    bool init(const DaqConfig& cfg, std::string& err);
    void shutdown();
    bool fire(std::string& err);
    bool isReady() const;

private:
    DaqConfig cfg_;
    bool ready_;

#ifdef HAVE_NIDAQMX
    void* task_;
#endif
};
