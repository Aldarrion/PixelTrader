#pragma once

#include "Render/VkTypes.h"
#include "Common/Enums.h"
#include "Common/Types.h"

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

