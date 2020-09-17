#include "WebRtcTool.h"
#include "vhall_sdk_log.h"
#include "pictureDecoder.h"
#include "ffmpeg_demuxer.h"
using namespace webrtc_sdk;

WebRtcTool::WebRtcTool()
{
}


WebRtcTool::~WebRtcTool()
{
}

int WebRtcTool::GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList) {
    LOGD_SDK(" GetMicDevices");
    vhall::VHTools vh;
    std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = vh.AudioRecordingDevicesList();
    for (int i = 0; i < micList.size(); i++) {
        std::shared_ptr<vhall::AudioDevProperty> mic = micList.at(i);
        vhall::AudioDevProperty info;
        info.mDevGuid = mic->mDevGuid;
        info.mDevName = mic->mDevName;
        info.mIndex = i;
        info.mFormat = mic->mFormat;
        info.m_sDeviceType = mic->m_sDeviceType;
        if (info.m_sDeviceType != vhall::TYPE_DSHOW_AUDIO) {
            micDevList.push_back(info);
        }
        LOGD_SDK("GetMicDevices name:%s index:%d  devid:%s", mic->mDevName.c_str(), mic->mIndex, mic->mDevGuid.c_str());
    }
    LOGD_SDK("GetMicDevices micDevList:%d", micDevList.size());
    return vlive::VhallLive_OK;
}

void WebRtcTool::ListenAudioCaptrue() {
    mAudioDeviceDispatcher = vhall::AdmConfig::GetAudioDeviceEventDispatcher();
    if (mAudioDeviceDispatcher) {
        mAudioDeviceDispatcher->AddEventListener(AUDIO_CAPTURE_ERROR, [&](vhall::BaseEvent &Event)->void {
            vhall::AudioDeviceEvent* event = dynamic_cast<vhall::AudioDeviceEvent*>(&Event);
            if (nullptr == event) {
                return;
            }
            CTaskEvent task;
            task.mEventType = TaskEventEnum_CaptureLocalStream;
            task.mErrMsg = AUDIO_CAPTURE_ERROR;
            WorkThread::PostTaskEvent(task);
        });
    }
}

bool WebRtcTool::HasVideoOrAudioDev() {
    vhall::VHTools vh;
    const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
    if (CameraList.size() > 0) {
        return true;
    }
    std::vector<std::shared_ptr<vhall::AudioDevProperty>> micList = vh.AudioRecordingDevicesList();
    if (micList.size() > 0) {
        return true;
    }
    return false;
}

int WebRtcTool::GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList) {
    vhall::VHTools vh;
    std::vector<std::shared_ptr<vhall::AudioDevProperty>> playList = vh.AudioPlayoutDevicesList();
    for (int i = 0; i < playList.size(); i++) {
        std::shared_ptr<vhall::AudioDevProperty> play = playList.at(i);
        vhall::AudioDevProperty info;
        info.mDevGuid = play->mDevGuid;
        info.mDevName = play->mDevName;
        info.mIndex = play->mIndex;
        info.mFormat = play->mFormat;
        playerDevList.push_back(info);
        LOGD_SDK("GetPlayerDevices name:%s index:%d devID:%s", info.mDevName.c_str(), play->mIndex, play->mDevGuid.c_str());
    }
    LOGD_SDK(" micDevList:%d", playerDevList.size());
    return vlive::VhallLive_OK;
}

int WebRtcTool::GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras) {
    vhall::VHTools vh;
    const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
    for (int i = 0; i < CameraList.size(); i++) {
        std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
        vhall::VideoDevProperty info;
        info.mDevId = videoDev->mDevId;
        info.mDevName = videoDev->mDevName;
        info.mIndex = videoDev->mIndex;
        LOGD_SDK("GetCameraDevices name:%s", videoDev->mDevName.c_str());
        for (int i = 0; i < videoDev->mDevFormatList.size(); i++) {
            std::shared_ptr<vhall::VideoFormat> cap = videoDev->mDevFormatList.at(i);
            std::shared_ptr<vhall::VideoFormat> format = std::make_shared<vhall::VideoFormat>();
            format->height = cap->height;
            format->width = cap->width;
            format->maxFPS = cap->maxFPS;
            format->videoType = (int)cap->videoType;
            format->interlaced = cap->interlaced;
            info.mDevFormatList.push_back(format);
            LOGD_SDK("GetCameraDevices format width:%d heigth:%d", format->width, format->height);
        }
        cameras.push_back(info);
    }
    LOGD_SDK(" GetCameraDevices :%d", cameras.size());
    return vlive::VhallLive_OK;
}

int WebRtcTool::GetCameraDevDetails(std::list<vlive::CameraDetailsInfo> &cameraDetails) {
    vhall::VHTools vh;
    const std::vector<std::shared_ptr<vhall::VideoDevProperty>> CameraList = vh.VideoInDeviceList();
    for (int i = 0; i < CameraList.size(); i++) {
        std::shared_ptr<vhall::VideoDevProperty> videoDev = CameraList.at(i);
        vlive::CameraDetailsInfo cameraInfo;
        cameraInfo.mDevId = videoDev->mDevId;
        cameraInfo.mDevName = videoDev->mDevName;
        cameraInfo.mIndex = videoDev->mIndex;
        std::unordered_map<VideoProfileIndex, bool> profileMap = vh.GetSupportIndex(videoDev->mDevFormatList);
        std::unordered_map<VideoProfileIndex, bool>::iterator iter = profileMap.begin();
        VideoProfileList mVideoProfiles;
        while (iter != profileMap.end()) {
            if (iter->second) {
                std::shared_ptr<VideoProfile> profile = mVideoProfiles.GetProfile(iter->first);
                VideoProfile profileTemp;
                profileTemp.mIndex = profile->mIndex;
                profileTemp.mWidth = profile->mWidth;
                profileTemp.mHeight = profile->mHeight;
                profileTemp.mRatioNum = profile->mRatioNum;
                profileTemp.mRatioDen = profile->mRatioDen;
                profileTemp.mMaxFrameRate = profile->mMaxFrameRate;
                profileTemp.mMinFrameRate = profile->mMinFrameRate;
                profileTemp.mMaxBitRate = profile->mMaxBitRate;
                profileTemp.mMinBitRate = profile->mMinBitRate;
                cameraInfo.profileList.push_back(profileTemp);
                LOGD_SDK("width :%d  height", profile->mWidth, profile->mHeight);
            }
            iter++;
        }
        cameraDetails.push_back(cameraInfo);
    }
    LOGD_SDK(" GetCameraDevDetails :%d", cameraDetails.size());
    return vlive::VhallLive_OK;
}

int WebRtcTool::CheckPicEffectiveness(const std::string filePath) {
    vhall::PictureDecoder dec;
    std::shared_ptr<vhall::MediaCaptureInfo> mediaInfo = std::make_shared<vhall::MediaCaptureInfo>();
    std::shared_ptr<vhall::I420Data> mI420Data = std::make_shared<vhall::I420Data>();
    int nRet = dec.Decode(filePath.c_str(), mI420Data->mData, mI420Data->width, mI420Data->height);
    return nRet;
}

int WebRtcTool::GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight) {
    vhall::FFmpegDemuxer ffmpegFileDemuxer;
    int nRet = ffmpegFileDemuxer.Init(filePath.c_str());
    if (nRet == 0) {
        if (ffmpegFileDemuxer.GetVideoPar() != NULL) {
            srcWidth = ffmpegFileDemuxer.GetVideoPar()->width;
            srcHeight = ffmpegFileDemuxer.GetVideoPar()->height;
        }
    }
    return nRet;
}