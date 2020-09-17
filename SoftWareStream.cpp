#include "SoftWareStream.h"
#include "vhall_sdk_log.h"
#include "vlive_def.h"
#include "pictureDecoder.h"
#include "ffmpeg_demuxer.h"
using namespace webrtc_sdk;


SoftWareStream::SoftWareStream()
{
}


SoftWareStream::~SoftWareStream()
{
}

int SoftWareStream::SelectSource(int64_t index) {
    if (mWebRtcStream) {
        mWebRtcStream->SelectSource(index);
        return vlive::VhallLive_AlreadyCapture;
    }
    else {
        mStreamConfig.mAudio = false;
        mStreamConfig.mData = true;
        mStreamConfig.mVideo = true;
        mStreamConfig.mStreamType = vlive::VHStreamType_SoftWare;
        mStreamConfig.mVideoCodecPreference = "VP8";
        mStreamConfig.videoOpt.mProfileIndex = VIDEO_PROFILE_720P_1_15F;
        LOGD_SDK("StreamConfig width:%d height:%d fps:%d wb:%d", mStreamConfig.videoOpt.maxWidth, mStreamConfig.videoOpt.maxHeight, mStreamConfig.videoOpt.maxFrameRate, mStreamConfig.videoOpt.minVideoBW);
        LOGD_SDK("make_shared StreamConfig");
        mbIsCapture = true;
        mStreamConfig.mLocal = true;
        mWebRtcStream = std::make_shared<vhall::VHStream>(mStreamConfig);
        if (mWebRtcStream) {
            LOGD_SDK("index:%d", index);
            mWebRtcStream->Init();
            int64_t iTemp = index;
            //当调用软件共享成功。
            mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&, iTemp](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_ACCEPTED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureSoftwareStream;
                task.mErrMsg = ACCESS_ACCEPTED;
                task.devIndex = iTemp;
                WorkThread::PostTaskEvent(task);

            });
            //当调用软件共享失败。
            mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {//错误提醒
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureSoftwareStream;
                task.mErrMsg = ACCESS_DENIED;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->Getlocalstream();
        }
        LOGD_SDK("VhallLive_NO_OBJ");
        return vlive::VhallLive_NO_OBJ;
    }

    LOGD_SDK("");
    return vlive::VhallLive_OK;
}