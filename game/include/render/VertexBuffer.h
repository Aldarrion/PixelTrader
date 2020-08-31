#pragma once

#include "VkTypes.h"
#include "Enums.h"
#include "Types.h"

namespace hs
{

class VertexBuffer
{
public:
    VertexBuffer(uint size);

    RESULT Init();
    VkBuffer GetBuffer() const;
    void* Map();
    void Unmap();

private:
    VkBuffer        buffer_;
    VmaAllocation   allocation_;
    uint            size_;
};

}
