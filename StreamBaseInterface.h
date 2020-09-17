#pragma once

#include "signalling/vh_connection.h"
#include "signalling/vh_data_message.h"
#include "signalling/vh_events.h"
#include "signalling/vh_room.h"
#include "signalling/vh_tools.h"
#include "signalling/vh_stream.h" 
#include "signalling/tool/AudioPlayDeviceModule.h"
#include "signalling/tool/AudioSendFrame.h"
#include "CTaskEvent.h"
#include <windows.h>
#include <atomic>
#include <thread>
#include "thread_lock.h"
#include "../WorkThread.h"

class StreamBase
{
public:  
    StreamBase();
    virtual ~StreamBase();
    void ReleaseStream();

    bool HasVideo();
    bool HasAudio();

    void StartRender(void* wnd);

public:
    std::shared_ptr<vhall::VHStream> mWebRtcStream = nullptr;
    std::atomic_bool mbIsCapture = false;
    std::string mStreamId;
    HWND mStreamWnd;
    
    vhall::StreamMsg mStreamConfig;

};

