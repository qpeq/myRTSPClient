#include <iostream>
#include "stream_client_state.h"
#include "my_media_sink.h"
#include "my_rtsp_client.h"

const static char* APP_NAME = "MyRTSPClient";
const static uint8_t VERBOSITY_LEVEL = 1;

UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
    return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
    return env << subsession.mediumName() << "/" << subsession.codecName();
}

struct MyRTSPClient::MyRTSPClientImpl
{
    MyRTSPClientImpl() = default;
    ~MyRTSPClientImpl() = default;

    StreamClientState scs;

    static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
        do {
            UsageEnvironment& env = rtspClient->envir();
            StreamClientState& scs = dynamic_cast<MyRTSPClient*>(rtspClient)->imp_->scs;

            if (resultCode != 0) {
                env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
                delete[] resultString;
                break;
            }

            char* const sdpDescription = resultString;
            env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

            // Create a media session object from this SDP description:
            scs.session = MediaSession::createNew(env, sdpDescription);
            delete[] sdpDescription; // because we don't need it anymore
            if (scs.session == nullptr) {
                env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
                break;
            } else if (!scs.session->hasSubsessions()) {
                env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
                break;
            }

            // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
            // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
            // (Each 'subsession' will have its own data source.)
            scs.iter = new MediaSubsessionIterator(*scs.session);
             setupNextSubsession(rtspClient);
            return;
        } while (0);

        // An unrecoverable error occurred with this stream.
        MyRTSPClientImpl::shutdownStream(rtspClient);
    }

    static void setupNextSubsession(RTSPClient* rtspClient) {
         UsageEnvironment& env = rtspClient->envir();
         StreamClientState& scs = dynamic_cast<MyRTSPClient*>(rtspClient)->imp_->scs;

         scs.subsession = scs.iter->next();
         if (scs.subsession != NULL) {
             if (!scs.subsession->initiate()) {
                 env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
                 setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
             } else {
                 env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
                 if (scs.subsession->rtcpIsMuxed()) {
                     env << "client port " << scs.subsession->clientPortNum();
                 } else {
                     env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
                 }
                 env << ")\n";

                 // Continue setting up this subsession, by sending a RTSP "SETUP" command:
                 rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, false, false);
             }
             return;
         }

         // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
         if (scs.session->absStartTime() != NULL) {
             // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
//             rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
         } else {
             scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
             rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
         }
    }

    static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
         do {
             UsageEnvironment& env = rtspClient->envir();
             StreamClientState& scs = dynamic_cast<MyRTSPClient*>(rtspClient)->imp_->scs;

             if (resultCode != 0) {
                 env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
                 break;
             }

             env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
             if (scs.subsession->rtcpIsMuxed()) {
                 env << "client port " << scs.subsession->clientPortNum();
             } else {
                 env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
             }
             env << ")\n";

             // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
             // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
             // after we've sent a RTSP "PLAY" command.)

             scs.subsession->sink = MyMediaSink::createNew(env, *scs.subsession, rtspClient->url());
             // perhaps use your own custom "MediaSink" subclass instead
             if (scs.subsession->sink == NULL) {
                 env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
                 break;
             }

             env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
             scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
             scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), subsessionAfterPlaying, scs.subsession);
             // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
             if (scs.subsession->rtcpInstance() != nullptr) {
                 scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
             }
         } while (0);
         delete[] resultString;

         setupNextSubsession(rtspClient);
    }

    static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
        Boolean success = False;

        do {
            UsageEnvironment& env = rtspClient->envir();
            StreamClientState& scs = dynamic_cast<MyRTSPClient*>(rtspClient)->imp_->scs;

            if (resultCode != 0) {
                env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
                break;
            }

            // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
            // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
            // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
            // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
            if (scs.duration > 0) {
                unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
                scs.duration += delaySlop;
                unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
                scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
            }

            env << *rtspClient << "Started playing session";
            if (scs.duration > 0) {
                env << " (for up to " << scs.duration << " seconds)";
            }
            env << "...\n";

            success = True;
        } while (0);
        delete[] resultString;

        if (!success) {
            // An unrecoverable error occurred with this stream.
            shutdownStream(rtspClient);
        }
    }

    static void shutdownStream(RTSPClient* rtspClient, int exitCode = 1) {
        UsageEnvironment& env = rtspClient->envir();
        StreamClientState& scs = dynamic_cast<MyRTSPClient*>(rtspClient)->imp_->scs;

        if (scs.session != nullptr) {
            Boolean someSubsessionsWereActive = False;
            MediaSubsessionIterator iter(*scs.session);
            MediaSubsession* subsession;

            while ((subsession = iter.next()) != nullptr) {
                if (subsession->sink != nullptr) {
                    Medium::close(subsession->sink);
                    subsession->sink = nullptr;

                    if (subsession->rtcpInstance() != nullptr) {
                        subsession->rtcpInstance()->setByeHandler(nullptr, nullptr); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                    }

                    someSubsessionsWereActive = True;
                }
            }

            if (someSubsessionsWereActive) {
                // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
                // Don't bother handling the response to the "TEARDOWN".
                rtspClient->sendTeardownCommand(*scs.session, nullptr);
            }
        }

        env << *rtspClient << "Closing the stream.\n";
        Medium::close(rtspClient);

        exit(exitCode);
    }

    static void streamTimerHandler(void* clientData) {
        auto rtspClient = (MyRTSPClient*)clientData;
        StreamClientState& scs = rtspClient->imp_->scs;

        scs.streamTimerTask = nullptr;

        // Shut down the stream:
        shutdownStream(rtspClient);
    }

    static void subsessionByeHandler(void* clientData, char const* reason) {
        MediaSubsession* subsession = (MediaSubsession*)clientData;
        RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
        UsageEnvironment& env = rtspClient->envir(); // alias

        env << *rtspClient << "Received RTCP \"BYE\"";
        if (reason != nullptr) {
            env << " (reason:\"" << reason << "\")";
            delete[] (char*)reason;
        }
        env << " on \"" << *subsession << "\" subsession\n";

        // Now act as if the subsession had closed:
        subsessionAfterPlaying(subsession);
    }

    static void subsessionAfterPlaying(void* clientData) {
        MediaSubsession* subsession = (MediaSubsession*)clientData;
        RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

        // Begin by closing this subsession's stream:
        Medium::close(subsession->sink);
        subsession->sink = nullptr;

        // Next, check whether *all* subsessions' streams have now been closed:
        MediaSession& session = subsession->parentSession();
        MediaSubsessionIterator iter(session);
        while ((subsession = iter.next()) != nullptr) {
            if (subsession->sink != nullptr) return; // this subsession is still active
        }

        // All subsessions' streams have now been closed, so shutdown the client:
        shutdownStream(rtspClient);
    }
};

RTSPClient* MyRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL) {
    return new MyRTSPClient(env, rtspURL);
}

MyRTSPClient::MyRTSPClient(UsageEnvironment& env, char const* rtspURL)
    : RTSPClient(env,rtspURL, VERBOSITY_LEVEL, APP_NAME, 0, -1), imp_(new MyRTSPClientImpl)
{
    this->sendDescribeCommand(MyRTSPClientImpl::continueAfterDESCRIBE);
}
