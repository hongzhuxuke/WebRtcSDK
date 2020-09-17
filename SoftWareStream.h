#pragma once
#include "../StreamBaseInterface.h"
class SoftWareStream : public StreamBase
{
public:
    SoftWareStream();
    virtual ~SoftWareStream();
    int SelectSource(int64_t index);
};

