#pragma once
#include "../StreamBaseInterface.h"

class VideoDeviceStream : public StreamBase
{
public:
    VideoDeviceStream();
    virtual ~VideoDeviceStream();

    void StartLocalCapture(const std::string devId, bool hasVideo, bool hasAudio, VideoProfileIndex index, bool doublePush = false);
    int StartLocalCapturePicture(const std::string filePath, bool hasVideo, bool hasAudio, VideoProfileIndex index, bool doublePush);

    int OpenCamera();
    int CloseCamera();

    void ResetFlags();
public:

    std::atomic_int mCurrentCameraIndex = 0;
    std::string mCurrentCameraGuid;
    std::atomic_bool mbIsOpenCamera = true;
};

