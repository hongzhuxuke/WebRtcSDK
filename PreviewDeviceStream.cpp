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
            //����������ͷ���ᴥ��ACCESS_ACCEPTED�ص���
            mWebRtcStream->AddEventListener(ACCESS_ACCEPTED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_ACCEPTED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = ACCESS_ACCEPTED;
                WorkThread::PostTaskEvent(task);
            });

            // ��ȡ��������ͷʧ�ܼ����ص�
            mWebRtcStream->AddEventListener(ACCESS_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("ACCESS_DENIED");
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = ACCESS_DENIED;
                WorkThread::PostTaskEvent(task);
            });

            mWebRtcStream->AddEventListener(VIDEO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("VIDEO_DENIED");
                //��Ƶ�豸��ʧ�ܣ���������豸�Ƿ���������
                CTaskEvent task;
                task.mEventType = TaskEventEnum_PREVIE_PLAY;
                task.mErrMsg = VIDEO_DENIED;
                WorkThread::PostTaskEvent(task);
            });

            mWebRtcStream->AddEventListener(AUDIO_DENIED, [&](vhall::BaseEvent &Event)->void {
                LOGD_SDK("AUDIO_DENIED");
                //��Ƶ�豸�쳣�������ϵͳ��������Ƶ�豸����Ƶ������Ϊ44100��ʽ
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