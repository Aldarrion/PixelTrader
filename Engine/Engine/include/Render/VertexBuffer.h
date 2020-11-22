#pragma once

#include "Render/VkTypes.h"
#include "Render/VertexBufferEntry.h"

#include "Containers/Array.h"

#include "Common/Enums.h"
#include "Common/Types.h"

namespace hs
{

//------------------------------------------------------------------------------
class VertexBuffer
{
public:
    VertexBuffer(uint size);

    RESULT Init();
    void Free();

    VkBuffer GetBuffer() const;
    uint GetSize() const;

    void* Map();
    void Unmap();

private:
    VkBuffer        buffer_;
    VmaAllocation   allocation_;
    uint            size_;
};

//------------------------------------------------------------------------------
class VertexBufferCache
{
public:
    ~VertexBufferCache();

    RESULT Init();

    VertexBufferEntry BeginAlloc(uint size, uint align, void** data);
    void EndAlloc();

private:
    constexpr static uint BUFFER_SIZE = 512 * 1024;

    struct CacheEntry
    {
        VertexBuffer buffer_{ BUFFER_SIZE };
        uint64 safeToUseFrame_{};
        uint begin_{};
    };

    Array<CacheEntry> entries_;
};

}
