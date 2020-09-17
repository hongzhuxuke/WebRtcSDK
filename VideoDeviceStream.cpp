#include "VideoDeviceStream.h"
#include "vhall_sdk_log.h"
#include "pictureDecoder.h"
#include "vlive_def.h"


using namespace webrtc_sdk;
VideoDeviceStream::VideoDeviceStream()
{
    mStreamConfig.mAudio = true;
    mStreamConfig.mData = true;
    mStreamConfig.mVideo = true;
    mStreamConfig.mStreamType = vlive::VHStreamType_AVCapture;
    mStreamConfig.videoOpt.maxWidth = 640;
    mStreamConfig.videoOpt.minWidth = 640;
    mStreamConfig.videoOpt.maxHeight = 480;
    mStreamConfig.videoOpt.minHeight = 480;
    mStreamConfig.videoOpt.maxVideoBW = 500;
    mStreamConfig.videoOpt.maxFrameRate = 15;
    mStreamConfig.videoOpt.minFrameRate = 15;
    mStreamConfig.reconnect = true;
    mStreamConfig.videoOpt.lockRatio = true;
}


VideoDeviceStream::~VideoDeviceStream()
{
}

int VideoDeviceStream::StartLocalCapturePicture(const std::string filePath, bool hasVideo, bool hasAudio, VideoProfileIndex index, bool doublePush) {
    bool bRet = false;
    bool bNeedInit = false;
    mStreamConfig.mNumSpatialLayers = doublePush ? 2 : 0;
    mStreamConfig.mVideo = hasVideo;
    mStreamConfig.mAudio = hasAudio;
    mStreamConfig.videoOpt.mProfileIndex = index;
    mStreamConfig.mLocal = true;
    LOGD_SDK("get mLocalStreamThreadLock");
    if (mWebRtcStream) {
        mWebRtcStream->ResetCapability(mStreamConfig.videoOpt);
    }
    if (mWebRtcStream == nullptr) {
        LOGD_SDK(" make mStreamConfig ");
        mWebRtcStream = std::make_shared<vhall::VHStream>(mStreamConfig);
        if (mWebRtcStream) {
            //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
            mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_ACCEPTED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_ACCEPTED;
                WorkThread::PostTaskEvent(task);
            });

            // 获取本地摄像头失败监听回调
            mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_DENIED;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("STREAM_SOURCE_LOST");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = CAMERA_LOST;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("VIDEO_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = VIDEO_DENIED;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("AUDIO_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_ACCEPTED;
                WorkThread::PostTaskEvent(task);
            });
        }
        bNeedInit = true;
        LOGD_SDK("make new local stream");
    }
    if (mWebRtcStream) {
        mbIsCapture = true;
    }
    if (bNeedInit && mWebRtcStream) {
        //初始化
        mWebRtcStream->Init();
        LOGD_SDK("Init");
    }

    if (mWebRtcStream) {
        mbIsCapture = true;
        vhall::PictureDecoder dec;
        std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
        std::shared_ptr<vhall::I420Data> mI420Data = std::make_shared<vhall::I420Data>();
        dec.Decode(filePath.c_str(), mI420Data->mData, mI420Data->width, mI420Data->height);
        if (mI420Data->mData == nullptr) {

            return vlive::VhallLive_NO_OBJ;
        }
        else {
            mediaInfo->mI420Data = mI420Data;
            mWebRtcStream->mVideo = mStreamConfig.mVideo;
            mWebRtcStream->Getlocalstream(mediaInfo);
            LOGD_SDK("Getlocalstream");
        }
    }
    else {
        return vlive::VhallLive_NO_OBJ;
    }
    LOGD_SDK("VhallLive_OK");
    return vlive::VhallLive_OK;
}

int VideoDeviceStream::OpenCamera() {
    LOGD_SDK("OpenCamera");
    if (mWebRtcStream && mbIsCapture && (mWebRtcStream->mStreamType == vlive::VHStreamType_AVCapture)) {
        LOGD_SDK("Camera is open");
        mWebRtcStream->MuteVideo(false);
        mbIsOpenCamera = true;
        mWebRtcStream->play(mStreamWnd);
        return 0;
    }
    return -1;
}

int VideoDeviceStream::CloseCamera() {
    LOGD_SDK(" CloseCamera");
    if (mWebRtcStream && mWebRtcStream->mStreamType == vlive::VHStreamType_AVCapture) {
        mWebRtcStream->MuteVideo(true);
        mWebRtcStream->stop();
        mbIsOpenCamera = false;
        LOGD_SDK(" CloseCamera end");
        return 0;
    }
    LOGD_SDK(" CloseCamera end");
    return -1;
}


void VideoDeviceStream::StartLocalCapture(const std::string devId, bool hasVideo, bool hasAudio, VideoProfileIndex index, bool doublePush/* = false*/) {
    LOGD_SDK(" start");
    bool bRet = false;
    bool bNeedInit = false;
    mStreamConfig.mNumSpatialLayers = doublePush ? 2 : 0;
    mStreamConfig.mVideo = hasVideo;
    mStreamConfig.mAudio = hasAudio;
    mStreamConfig.videoOpt.mProfileIndex = index;
    mStreamConfig.mLocal = true;
    if (mWebRtcStream) {
        mWebRtcStream->ResetCapability(mStreamConfig.videoOpt);
    }
    mCurrentCameraGuid = devId;
    LOGD_SDK("get mLocalStreamThreadLock");
    if (mWebRtcStream == nullptr) {
        LOGD_SDK(" make mStreamConfig ");
        mWebRtcStream = std::make_shared<vhall::VHStream>(mStreamConfig);
        if (mWebRtcStream) {
            //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
            mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_ACCEPTED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_ACCEPTED;
                WorkThread::PostTaskEvent(task);
            });
            // 获取本地摄像头失败监听回调
            mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = ACCESS_DENIED;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->AddEventListener(CAMERA_LOST, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("STREAM_SOURCE_LOST");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = CAMERA_LOST;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = VIDEO_DENIED;
                WorkThread::PostTaskEvent(task);
            });
            mWebRtcStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("AUDIO_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_CaptureLocalStream;
                task.mErrMsg = AUDIO_DENIED;
                WorkThread::PostTaskEvent(task);
            });
        }
        bNeedInit = true;
        LOGD_SDK("make new local stream");
    }
    if (mWebRtcStream) {
        mbIsCapture = true;
    }
    if (bNeedInit && mWebRtcStream) {
        //初始化
        LOGD_SDK("Init");
        mWebRtcStream->Init();
    }

    if (mWebRtcStream) {
        LOGD_SDK("Getlocalstream");
        std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
        mediaInfo->VideoDevID = devId;
        mWebRtcStream->mVideo = mStreamConfig.mVideo;
        mWebRtcStream->Getlocalstream(mediaInfo);
        LOGD_SDK("end");
    }
    LOGD_SDK("VhallLive_OK");
    return;
}


void VideoDeviceStream::ResetFlags() {
    mStreamWnd = nullptr;
    mCurrentCameraIndex = 0;
    mCurrentCameraGuid.clear();
    mbIsCapture = false;
    mbIsOpenCamera = true;
    LOGD_SDK("StopLocalCapture ok");
}