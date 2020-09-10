#include "CAudioVolume.h"

int AudioEnergyCalc(short * audioData, int sampleNum) {
    if (audioData == nullptr || sampleNum <= 0) {
        return -1;
    }

    int		cnt = 0;
    int		sampleEnergy = 0;
    float	energySum = 0.0;
    int		retValue = 0;
    int sampleValue = 0;

    //sum loop
    for (cnt = 0; cnt < sampleNum; cnt++) {
        float curValue = abs(audioData[cnt]) / 32767.0;
        if (curValue > energySum) {
            energySum = curValue;
        }
    }
    retValue = (int)(energySum * 100 /*/ sampleNum*/);
    if (retValue < 0) {
        retValue = 0;
    }
    else if (retValue > 100) {
        retValue = 100;
    }
    return retValue;
}

CAudioVolume::CAudioVolume()
{
    mValue = 0;
}


CAudioVolume::~CAudioVolume()
{
}


void CAudioVolume::SendAudioData(const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel) {
    mValue = AudioEnergyCalc((short *)audio_data, number_frames_perChannel * number_of_channels);
}

int CAudioVolume::GetAudioVolume() {
    return mValue;
}