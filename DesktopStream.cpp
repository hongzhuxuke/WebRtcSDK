#include "DesktopStream.h"
#include "vhall_sdk_log.h"
#include "vlive_def.h"
using namespace webrtc_sdk;

DesktopStream::DesktopStream()
{
}


DesktopStream::~DesktopStream()
{
}

int DesktopStream::StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex) {
    mStreamConfig.mAudio = false;
    mStreamConfig.mData = true;
    mStreamConfig.mVideo = true;
    mStreamConfig.mStreamType = vlive::VHStreamType_Desktop;
    mStreamConfig.videoOpt.mProfileIndex = profileIndex;
    mStreamConfig.mLocal = true;
    mbIsCapture = true;
    int64_t iTempId = index;
    if (mWebRtcStream) {
        LOGD_SDK("SelectSource :%d", index);
        mWebRtcStream->SelectSource(index);
        return vlive::VhallLive_AlreadyCapture;
    }
    else {
        ReleaseStream();
        LOGD_SDK("make_shared StreamConfig");
        if (mWebRtcStream == nullptr) {
            mWebRtcStream = std::make_shared<vhall::VHStream>(mStreamConfig);
            if (mWebRtcStream) {
                LOGD_SDK("index:%d", index);
                mWebRtcStream->Init();
                //当调用桌面共享成功。
                mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&, iTempId](vhall::BaseEvent &Event)->void {
                    LOGD_SDK("ACCESS_ACCEPTED");
                    CTaskEvent task;
                    task.mEventType = TaskEventEnum_CaptureDesktopStream;
                    task.mErrMsg = ACCESS_ACCEPTED;
                    task.devIndex = iTempId;
                    WorkThread::PostTaskEvent(task);

                });
                //当调用桌面共享失败。
                mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {//错误提醒
                    CTaskEvent task;
                    task.mEventType = TaskEventEnum_CaptureDesktopStream;
                    task.mErrMsg = ACCESS_DENIED;
                    LOGD_SDK("ACCESS_DENIED");
                    WorkThread::PostTaskEvent(task);
                });
            }
        }
        if (mWebRtcStream) {
            mWebRtcStream->Getlocalstream();
        }
    }
    LOGD_SDK("");
    return vlive::VhallLive_OK;
}
