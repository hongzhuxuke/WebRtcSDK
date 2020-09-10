#pragma once
#include "signalling/tool/AudioSendFrame.h"
#include <atomic>
class CAudioVolume: public vhall::AudioSendFrame
{
public:
    CAudioVolume();
    ~CAudioVolume();

    virtual void SendAudioData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel);

    int GetAudioVolume();
private:
    std::atomic_int mValue;
};

