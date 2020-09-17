#include "WorkThread.h"
#include "../WebRTC/WebRtcSDK.h"
#include "vhall_sdk_log.h"

HANDLE WorkThread::gTaskEvent = nullptr;
HANDLE WorkThread::gThreadExitEvent = nullptr;
std::atomic_bool WorkThread::bThreadRun = true;
CThreadLock WorkThread::taskListMutex;
std::list<CTaskEvent> WorkThread::mtaskEventList;

using namespace webrtc_sdk;
WorkThread::WorkThread()
{
}


WorkThread::~WorkThread()
{
}

void WorkThread::ReisterProcessor(WebRtcSDK* processor) {
    mProcessorPtr = processor;
}

void WorkThread::Run() {
    LOGD_SDK("Run");
    bThreadRun = true;
    mProcessThread = new std::thread(WorkThread::ThreadProcess, mProcessorPtr);
    if (mProcessThread) {
        gTaskEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        gThreadExitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    LOGD_SDK("Run end");
}

DWORD WINAPI WorkThread::ThreadProcess(LPVOID p) {
    while (bThreadRun) {
        if (gTaskEvent) {
            DWORD ret = WaitForSingleObject(gTaskEvent, 2000);
            if (p) {
                CTaskEvent event;
                CThreadLockHandle lockHandle(&taskListMutex);
                if (mtaskEventList.size() > 0) {
                    event = mtaskEventList.front();
                    mtaskEventList.pop_front();
                }
                else {
                    continue;
                }

                WebRtcSDK* sdk = (WebRtcSDK*)(p);
                if (sdk) {
                    sdk->ProcessTask(event);
                }
            }
        }
    }
    if (gThreadExitEvent) {
        ::SetEvent(gThreadExitEvent);
    }
    return 0;
}

void WorkThread::Stop() {
    LOGD_SDK("WebRtcSDK::ExitThread");
    bThreadRun = false;
    if (gTaskEvent) {
        ::SetEvent(gTaskEvent);
    }
    if (mProcessThread) {
        WaitForSingleObject(gThreadExitEvent, 5000);
        ::CloseHandle(gThreadExitEvent);
        gThreadExitEvent = nullptr;
        LOGD_SDK("join");
        mProcessThread->join();
        LOGD_SDK("join end");
        delete mProcessThread;
        mProcessThread = nullptr;
    }
    if (gTaskEvent) {
        ::CloseHandle(gTaskEvent);
        gTaskEvent = nullptr;
    }
    LOGD_SDK("WebRtcSDK::end");
}

void WorkThread::PostTaskEvent(CTaskEvent& task) {
    if (bThreadRun) {
        CThreadLockHandle lockHandle(&taskListMutex);
        mtaskEventList.push_back(task);
        if (gTaskEvent) {
            ::SetEvent(gTaskEvent);
        }
    }
}
