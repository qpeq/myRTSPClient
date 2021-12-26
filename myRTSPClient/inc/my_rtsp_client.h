#pragma once
#include <memory>
#include <string>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "stream_client_state.h"

class MyRTSPClient : public RTSPClient
{
public:
    static RTSPClient* createNew(UsageEnvironment& env, char const* rtspURL);

protected:
    MyRTSPClient(UsageEnvironment& env, char const* rtspURL);
    virtual ~MyRTSPClient() = default;

private:
    struct MyRTSPClientImpl;
    std::unique_ptr<MyRTSPClientImpl> imp_;
};
