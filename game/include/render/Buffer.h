#pragma once

#include "render/VkTypes.h"
#include "common/Enums.h"
#include "common/Types.h"

namespace hs
{

class TempStagingBuffer
{
public:
    TempStagingBuffer(uint size);
    RESULT Allocate(void* data);

    VkBuffer GetBuffer() const;

private:
    VkBuffer buffer_;
    VmaAllocation allocation_;
    uint size_;
};

}

