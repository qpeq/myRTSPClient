#pragma once
#include <string>
#include <stdint.h>
#include <UsageEnvironment/include/Boolean.hh>
#include <liveMedia/include/MediaSession.hh>

uint32_t get_nal_unit_type(uint8_t* buf, unsigned int nLen);

class MyMediaSink: public MediaSink {
public:
    static MyMediaSink* createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId=nullptr);

private:
    MyMediaSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
    virtual ~MyMediaSink();

    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    virtual Boolean continuePlaying() override;

    uint8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    char* fStreamId;
    const static uint32_t ReceiveBufferSize = 100000;
    uint32_t gop_count = 0;
    std::string videoCodecName;
    std::string audioCodecName;
};
