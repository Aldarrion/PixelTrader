#pragma once

#include "render/VkTypes.h"
#include "common/Enums.h"
#include "common/Types.h"

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

    void Free();

private:
    VkBuffer        buffer_;
    VmaAllocation   allocation_;
    uint            size_;
};

}
