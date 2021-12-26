#include <iostream>
#include "my_rtsp_client.h"
#include "my_media_sink.h"
#include "stream_client_state.h"

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Please provide an url" << std::endl;
        return 0;
    }

    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    RTSPClient* rtspClient = MyRTSPClient::createNew(*env, argv[1]);
    if (nullptr == rtspClient) {
        *env << "Failed to create a RTSP client for URL \"" << argv[1] << "\": " << env->getResultMsg() << "\n";
        return 0;
    }

    char eventLoopWatchVariable = 0;
    env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
    
    return 0;
}
