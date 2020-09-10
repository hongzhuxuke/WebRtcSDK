#ifndef TaskEvent_HEAD_FILE
#define TaskEvent_HEAD_FILE
#include <iostream>
#include "video_profile.h"

enum TaskEventEnum
{
    TaskEventEnum_STREAM_LOST = 0, //流丢失
    TaskEventEnum_ADD_STREAM,
    TaskEventEnum_RE_SUBSCRIB,      //订阅流
    TaskEventEnum_SUBSCRIB_ERROR,      //订阅流失败
    TaskEventEnum_RECV_MEDIA_STREAM,      //订阅到插播或桌面共享视频流
   TaskEventEnum_ROOM_RECONNECTED,         //房间重新链接，重新订阅
   TaskEventEnum_SetSubscribe_PlayOutVol,   //设置订阅流扬声器音量
    TaskEventEnum_PREVIE_PLAY,
    TaskEventEnum_StartLocalCapture,
    TaskEventEnum_StartLocalAuxiliaryCapture,
    TaskEventEnum_StartLocalPicCapture,
    TaskEventEnum_StartPreviewLocalCapture,
    TaskEventEnum_ON_STREAM_MIXED,

    TaskEventEnum_SET_MIC_DEV,
    TaskEventEnum_SET_PLAYER_DEV,

    TaskEventEnum_CaptureLocalStream,
    TaskEventEnum_CaptureAuxiliaryLocalStream,
    TaskEventEnum_CaptureMediaFile,
    TaskEventEnum_CaptureSoftwareStream,
    TaskEventEnum_CaptureDesktopStream,
    TaskEventEnum_Capture_Audio_Err,
    TaskEventEnum_VHStreamType_Preview_Video,

    TaskEventEnum_STREAM_REPUBLISHED,
    TaskEventEnum_STREAM_FAILED,
    TaskEventEnum_STREAM_REMOVED,
    TaskEventEnum_STREAM_SUBSCRIBED_SUCCESS,
    TaskEventEnum_STREAM_RECV_LOST,

    TaskEventEnum_ROOM_CONNECTED,
    TaskEventEnum_ROOM_ERROR,
    TaskEventEnum_ROOM_RECONNECTING,
    TaskEventEnum_ROOM_RECOVER,
    TaskEventEnum_ROOM_NETWORKCHANGED,
};

class CTaskEvent
{
public:
    CTaskEvent();
    ~CTaskEvent();
public:

    bool doublePush = false;
    bool mHasVideo = false;
    bool mHasAudio = false;

    int mEventType = -1;
    int mSleepTime = 0;
    int devIndex = 0;
    int mStreamType = -1;
    int mErrorType = 0;
    int mMimulCastType = 0;
    int mVolume = 0;

    unsigned long long mSeekPos = 0;

    std::wstring mStreamIndex;
    std::string filePath;
    std::string devId;
    VideoProfileIndex index;

    std::string mStreamAttributes;
    std::string mErrMsg;
    std::string mJoinUid;
    std::string streamId;
    std::wstring mUserId;
    std::wstring desktopCaptureId;

    void* wnd = nullptr;
};
#endif

