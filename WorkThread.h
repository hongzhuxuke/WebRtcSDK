#pragma once
#include "CTaskEvent.h"
#include <windows.h>
#include <atomic>
#include <thread>
#include "thread_lock.h"

class WebRtcSDK;

class WorkThread
{
public:
    WorkThread();
    virtual ~WorkThread();

    void ReisterProcessor(WebRtcSDK* processor);
    void Run();
    void Stop();
    static void PostTaskEvent(CTaskEvent& task);
    static DWORD WINAPI ThreadProcess(LPVOID p);
private:
    WebRtcSDK* mProcessorPtr = nullptr;

    HANDLE mHThread = nullptr;
    std::thread* mProcessThread = nullptr;
    DWORD  threadId = 0;


    static HANDLE gTaskEvent;
    static HANDLE gThreadExitEvent;
    static std::atomic_bool bThreadRun;
    static CThreadLock taskListMutex;
    static std::list<CTaskEvent> mtaskEventList;
};

