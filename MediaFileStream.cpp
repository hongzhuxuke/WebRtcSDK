#include "MediaFileStream.h"
#include "vhall_sdk_log.h"
#include "vlive_def.h"
#include "pictureDecoder.h"
#include "ffmpeg_demuxer.h"
using namespace webrtc_sdk;



MediaFileStream::MediaFileStream()
{
}


MediaFileStream::~MediaFileStream()
{
}

int MediaFileStream::StartPlayMediaFile(std::string filePath, VideoProfileIndex profileIndex, long long seekPos) {
    LOGD_SDK("enter");
    mCurPlayFile = filePath;
    mStreamConfig.mAudio = true;
    mStreamConfig.mVideo = true;
    mStreamConfig.mStreamType = vlive::VHStreamType_MediaFile;
    if (mWebRtcStream) {
        LOGD_SDK("StopPushMediaFileStream");
        ReleaseStream();
        mbIsCapture = false;
    }
    mStreamConfig.videoOpt.mProfileIndex = profileIndex;
    mbIsCapture = true;
    int width = 0;
    int height = 0;
    vhall::FFmpegDemuxer ffmpegFileDemuxer;
    int nRet = ffmpegFileDemuxer.Init(mCurPlayFile.c_str());
    if (nRet == 0) {
        if (ffmpegFileDemuxer.GetVideoPar() != NULL) {
            width = ffmpegFileDemuxer.GetVideoPar()->width;
            height = ffmpegFileDemuxer.GetVideoPar()->height;
        }
        mStreamConfig.mAudio = ffmpegFileDemuxer.hasAudio;
        mStreamConfig.mVideo = ffmpegFileDemuxer.hasVideo;
    }

    LOGD_SDK("file w:%d h:%d audio:%d video:%d", width, height, mStreamConfig.mAudio, mStreamConfig.mVideo);
    mWebRtcStream.reset(new vhall::VHStream(mStreamConfig));
    if (mWebRtcStream) {
        mWebRtcStream->SetEventCallBack(OnPlayMediaFileCb, this);
        //开启插播流
        mWebRtcStream->Init();
        //当调用插播成功。
        mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&, seekPos](vhall::BaseEvent &Event)->void {
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureMediaFile;
            task.mErrMsg = ACCESS_ACCEPTED;
            task.mSeekPos = seekPos;
            WorkThread::PostTaskEvent(task);
        });
        // 获取本地插播失败监听回调
        mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureMediaFile;
            task.mErrMsg = ACCESS_DENIED;
            WorkThread::PostTaskEvent(task);
        });
        mWebRtcStream->Getlocalstream();
    }
    else {
        LOGD_SDK("VhallLive_NO_OBJ");
        return vlive::VhallLive_NO_OBJ;
    }
    LOGD_SDK("");
    return vlive::VhallLive_OK;
}

void MediaFileStream::OnPlayMediaFileCb(int code, void *param) {
    if (param) {
        LOGD_SDK("code :%d", code);
    }
}

bool MediaFileStream::FilePlay() {
    bool bRet = false;
    if (mWebRtcStream) {
        bRet = mWebRtcStream->FilePlay(mCurPlayFile.c_str());
        LOGD_SDK("bRet:%d", bRet);
        if (bRet) {
            mbIsCapture = true;
        }
        else {
            mbIsCapture = false;
        }
    }
    return bRet;
}

void MediaFileStream::ChangeMediaFileProfile(VideoProfileIndex profileIndex) {
    if (mWebRtcStream) {
        mStreamConfig.videoOpt.mProfileIndex = profileIndex;
        mWebRtcStream->ResetCapability(mStreamConfig.videoOpt);
    }
}