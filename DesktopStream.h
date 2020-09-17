#pragma once
#include "../StreamBaseInterface.h"

class DesktopStream : public StreamBase
{
public:
    DesktopStream();
    virtual ~DesktopStream();

    int StartDesktopCapture(int64_t index, VideoProfileIndex profileIndex);
};

