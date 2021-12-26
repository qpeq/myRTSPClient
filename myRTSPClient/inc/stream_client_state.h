#pragma once
#include "liveMedia.hh"
#include "UsageEnvironment.hh"

class StreamClientState {
public:
    StreamClientState() = default;
    virtual ~StreamClientState() {
        delete iter;
        if (session != NULL) {
            // We also need to delete "session", and unschedule "streamTimerTask" (if set)
            UsageEnvironment& env = session->envir(); // alias

            env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
            Medium::close(session);
        }
    }

public:
    MediaSubsessionIterator* iter;
    MediaSession* session;
    MediaSubsession* subsession;
    TaskToken streamTimerTask;
    double duration;
};
