#pragma once

#include "VkTypes.h"
#include "Enums.h"
#include "Types.h"

namespace hs
{

class StagingBuffer
{
public:
    StagingBuffer(uint size);
    RESULT Allocate(void* data);

    VkBuffer GetBuffer() const;

private:
    VkBuffer buffer_;
    VmaAllocation allocation_;
    uint size_;
};

}

