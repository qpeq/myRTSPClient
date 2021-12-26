#include <cstring>
#include "my_media_sink.h"
#include "get_nal_unit_type.h"


const uint8_t NALU_TYPE_NON_IDR_FRAME = 1;
const uint8_t NALU_TYPE_IDR_FRAME = 5;

MyMediaSink* MyMediaSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
    return new MyMediaSink(env, subsession, streamId);
}

MyMediaSink::MyMediaSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env), fSubsession(subsession) {
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[ReceiveBufferSize];
}

MyMediaSink::~MyMediaSink() {
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

Boolean MyMediaSink::continuePlaying() {
    if (nullptr == fSource) return false;

    fSource->getNextFrame(fReceiveBuffer, ReceiveBufferSize, afterGettingFrame, this, onSourceClosure, this);
    return true;
}

void MyMediaSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
    auto sink = (MyMediaSink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void MyMediaSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
    auto mediumName = fSubsession.mediumName();
    if (strcmp(mediumName, "video") == 0) {
        if (videoCodecName.empty()) {
            videoCodecName = fSubsession.codecName();
            envir() << "\rVideo codec: " << videoCodecName.c_str() << "\n";
        }

        if (videoCodecName == "H264") {
            uint32_t nalu_type = get_nal_unit_type(fReceiveBuffer, frameSize);
            if (NALU_TYPE_IDR_FRAME == nalu_type) {
                if (gop_count > 1) {
                    envir() << "GoP size:" << gop_count << "\n";
                    gop_count = 1;
                } else {
                    gop_count++;
                }
            }
            else if (NALU_TYPE_NON_IDR_FRAME == nalu_type) {
                gop_count++;
            }
            // Just ignore other nal unit types
        }
    }
    else if (strcmp(mediumName, "audio") == 0) {
        if (audioCodecName.empty()) {
            audioCodecName = fSubsession.codecName();
            envir() << "\rAudio codec: " << audioCodecName.c_str() << "\n";
        }
    }

    continuePlaying();
}
