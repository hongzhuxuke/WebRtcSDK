#pragma once
#include "../StreamBaseInterface.h"
#include "vlive_def.h"
#include "signalling\BaseStack\AdmConfig.h"

class WebRtcTool
{
public:
    WebRtcTool();
    virtual ~WebRtcTool();

    int GetCameraDevDetails(std::list<vlive::CameraDetailsInfo> &cameraDetails);
    int GetCameraDevices(std::list<vhall::VideoDevProperty> &cameras);
    int GetMicDevices(std::list<vhall::AudioDevProperty> &micDevList);
    int GetPlayerDevices(std::list<vhall::AudioDevProperty> &playerDevList);
    bool HasVideoOrAudioDev();
    void ListenAudioCaptrue();

    int CheckPicEffectiveness(const std::string filePath);
    int GetPlayMeidaFileWidthAndHeight(std::string filePath, int& srcWidth, int &srcHeight);
private:
    std::shared_ptr<vhall::EventDispatcher> mAudioDeviceDispatcher = nullptr;
};

