#pragma once
#include "../StreamBaseInterface.h"

class MediaFileStream : public StreamBase
{
public:
    MediaFileStream();
    virtual ~MediaFileStream();

    int StartPlayMediaFile(std::string filePath, VideoProfileIndex profileIndex, long long seekPos);
    void ChangeMediaFileProfile(VideoProfileIndex profileIndex);
    bool FilePlay();
private:
    static void OnPlayMediaFileCb(int code, void *param);
public:
    std::string mCurPlayFile;
};

