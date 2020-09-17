#include "PreviewDeviceStream.h"
#include "vhall_sdk_log.h"
#include "vlive_def.h"
#include "pictureDecoder.h"
#include "ffmpeg_demuxer.h"
using namespace webrtc_sdk;


PreviewDeviceStream::PreviewDeviceStream()
{
}


PreviewDeviceStream::~PreviewDeviceStream()
{
}

int PreviewDeviceStream::HandlePreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/) {
    bool bRet = false;
    std::string camera_guid = devGuid;
    mStreamWnd = HWND(wnd);
    mStreamConfig.mVideo = devGuid.length() > 0 ? true : false;
    mStreamConfig.mAudio = false;
    mStreamConfig.videoOpt.mProfileIndex = index;
    LOGD_SDK("get mPreviewStreamThreadLock");

    bool bInit = false;
    if (mWebRtcStream == nullptr) {
        LOGD_SDK("RelaseStream and make_shared VHStream");
        mWebRtcStream = std::make_shared<vhall::VHStream>(mStreamConfig);
        if (mWebRtcStream) {
            //当调用摄像头，会触发ACCESS_ACCEPTED回掉。
            mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_ACCEPTED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = ACCESS_ACCEPTED;
                WorkThread::PostTaskEvent(task);
            });

            // 获取本地摄像头失败监听回调
            mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = ACCESS_DENIED;
                WorkThread::PostTaskEvent(task);
            });

            mWebRtcStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("VIDEO_DENIED");
                //视频设备打开失败，请您检测设备是否连接正常
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = VIDEO_DENIED;
                WorkThread::PostTaskEvent(task);
            });

            mWebRtcStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("AUDIO_DENIED");
                //音频设备异常，请调整系统设置中音频设备的音频采样率为44100格式
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = AUDIO_DENIED;
                WorkThread::PostTaskEvent(task);
            });
            bInit = true;
        }
    }
    if (bInit) {
        mWebRtcStream->Init();
    }

    if (mWebRtcStream) {
        mWebRtcStream->SetAudioListener(&mPreviewAudioVolume);
    }

    if (mWebRtcStream) {
        std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
        mediaInfo->VideoDevID = camera_guid;
        mWebRtcStream->Getlocalstream(mediaInfo);
    }
    LOGD_SDK("VhallLive_OK");
    return vlive::VhallLive_OK;
}

int PreviewDeviceStream::GetAudioVolume() {
    return mPreviewAudioVolume.GetAudioVolume();
}