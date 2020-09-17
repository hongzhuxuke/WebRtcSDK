#pragma once
#include "../StreamBaseInterface.h"
#include "CAudioVolume.h"
class PreviewDeviceStream : public StreamBase
{
public:
    PreviewDeviceStream();
    virtual ~PreviewDeviceStream();

    int HandlePreviewCamera(void* wnd, const std::string& devGuid, VideoProfileIndex index, int micIndex /*= -1*/);

    int  GetAudioVolume();
private:
    CAudioVolume mPreviewAudioVolume;
};

