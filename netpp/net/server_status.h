#pragma once

#include <atomic>
#include <string>

#ifndef H_CASE_STRING_BIGIN
#define H_CASE_STRING_BIGIN(state) switch(state){
#define H_CASE_STRING(state) case state:return #state;break;
#define H_CASE_STRING_END()  default:return "Unknown";break;}
#endif

namespace netpp {
class ServerStatus {
public:
    enum Status {
        kNull = 0,
        kInitializing = 1,
        kInitialized = 2,
        kStarting = 3,
        kRunning = 4,
        kStopping = 5,
        kStopped = 6,
    };

    enum SubStatus {
        kSubStatusNull = 0,
        kStoppingListener = 1,
        kStoppingThreadPool = 2,
    };

    std::string StatusToString() const {
        H_CASE_STRING_BIGIN(status_.load());
        H_CASE_STRING(kNull);
        H_CASE_STRING(kInitialized);
        H_CASE_STRING(kRunning);
        H_CASE_STRING(kStopping);
        H_CASE_STRING(kStopped);
        H_CASE_STRING_END();
    }

    bool isRunning() const {
        return status_.load() == kRunning;
    }

    bool isStopped() const {
        return status_.load() == kStopped;
    }

    bool isStopping() const {
        return status_.load() == kStopping;
    }

protected:
    std::atomic<Status> status_ = { kNull };
    std::atomic<SubStatus> substatus_ = { kSubStatusNull };
};
}