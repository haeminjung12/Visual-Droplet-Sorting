#include "daq_trigger.h"

#include <chrono>
#include <string>
#include <thread>

#ifdef HAVE_NIDAQMX
#include <NIDAQmx.h>

static std::string formatDaqError(const char* label, int32 err) {
    char buf[2048] = {};
    DAQmxGetExtendedErrorInfo(buf, static_cast<uInt32>(sizeof(buf)));
    std::string msg = label;
    msg += " (error ";
    msg += std::to_string(static_cast<int>(err));
    msg += ")";
    if (buf[0] != '\0') {
        msg += ": ";
        msg += buf;
    }
    return msg;
}
#endif

DaqTrigger::DaqTrigger()
    : ready_(false)
#ifdef HAVE_NIDAQMX
    , task_(nullptr)
#endif
{}

DaqTrigger::~DaqTrigger() {
    shutdown();
}

bool DaqTrigger::init(const DaqConfig& cfg, std::string& err) {
    cfg_ = cfg;
    ready_ = false;
    shutdown();

    if (cfg_.mode == TriggerMode::None) {
        ready_ = true;
        return true;
    }

#ifdef HAVE_NIDAQMX
    TaskHandle task = nullptr;
    if (cfg_.mode == TriggerMode::Digital) {
        std::string chan = cfg_.device + "/" + cfg_.line;
        int32 error = DAQmxCreateTask("", &task);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxCreateTask failed", error);
            return false;
        }
        error = DAQmxCreateDOChan(task, chan.c_str(), "", DAQmx_Val_ChanPerLine);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxCreateDOChan failed", error);
            DAQmxClearTask(task);
            return false;
        }
        error = DAQmxStartTask(task);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxStartTask failed", error);
            DAQmxClearTask(task);
            return false;
        }
    } else if (cfg_.mode == TriggerMode::Counter) {
        std::string chan = cfg_.device + "/" + cfg_.counter;
        double high = cfg_.pulseHighMs / 1000.0;
        double low = cfg_.pulseLowMs / 1000.0;
        int32 error = DAQmxCreateTask("", &task);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxCreateTask failed", error);
            return false;
        }
        error = DAQmxCreateCOPulseChanTime(task, chan.c_str(), "", DAQmx_Val_Seconds,
                                           DAQmx_Val_Low, 0.0, high, low);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxCreateCOPulseChanTime failed", error);
            DAQmxClearTask(task);
            return false;
        }
        error = DAQmxCfgImplicitTiming(task, DAQmx_Val_FiniteSamps, 1);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxCfgImplicitTiming failed", error);
            DAQmxClearTask(task);
            return false;
        }
    }

    task_ = task;
    ready_ = true;
    return true;
#else
    (void)cfg_;
    err = "NI-DAQmx not available at build time";
    return false;
#endif
}

void DaqTrigger::shutdown() {
#ifdef HAVE_NIDAQMX
    if (task_) {
        TaskHandle task = static_cast<TaskHandle>(task_);
        DAQmxStopTask(task);
        DAQmxClearTask(task);
        task_ = nullptr;
    }
#endif
    ready_ = false;
}

bool DaqTrigger::fire(std::string& err) {
    if (!ready_) {
        err = "DAQ trigger not initialized";
        return false;
    }
    if (cfg_.mode == TriggerMode::None) return true;

#ifdef HAVE_NIDAQMX
    TaskHandle task = static_cast<TaskHandle>(task_);
    if (cfg_.mode == TriggerMode::Digital) {
        uInt8 data[1];
        int32 written = 0;
        data[0] = 1;
        int32 error = DAQmxWriteDigitalLines(task, 1, 1, 1.0, DAQmx_Val_GroupByChannel, data, &written, nullptr);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxWriteDigitalLines high failed", error);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(cfg_.pulseHighMs));
        data[0] = 0;
        error = DAQmxWriteDigitalLines(task, 1, 1, 1.0, DAQmx_Val_GroupByChannel, data, &written, nullptr);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxWriteDigitalLines low failed", error);
            return false;
        }
        return true;
    }

    if (cfg_.mode == TriggerMode::Counter) {
        int32 error = DAQmxStartTask(task);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxStartTask failed", error);
            return false;
        }
        error = DAQmxWaitUntilTaskDone(task, 5.0);
        if (DAQmxFailed(error)) {
            err = formatDaqError("DAQmxWaitUntilTaskDone failed", error);
            DAQmxStopTask(task);
            return false;
        }
        DAQmxStopTask(task);
        return true;
    }

    err = "unknown trigger mode";
    return false;
#else
    err = "NI-DAQmx not available at build time";
    return false;
#endif
}

bool DaqTrigger::isReady() const {
    return ready_;
}
