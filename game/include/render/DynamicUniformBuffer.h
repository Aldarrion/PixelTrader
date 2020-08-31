#pragma once

#include "DynamicUniformBufferEntry.h"

#include "Types.h"
#include "VkTypes.h"
#include "Enums.h"
#include "Array.h"

namespace hs
{

//------------------------------------------------------------------------------
class DynamicUniformBuffer
{
public:
    DynamicUniformBuffer(uint size);

    RESULT Init();

    void* Map();
    void Unmap();

    VkBuffer GetBuffer() const;
    uint GetSize() const;

private:
    VkBuffer        buffer_{};
    VmaAllocation   allocation_{};
    VkBufferView    view_{};
    uint            size_;
};

//------------------------------------------------------------------------------
class DynamicUBOCache
{
public:
    RESULT Init();

    DynamicUBOEntry BeginAlloc(uint size, void** data);
    void EndAlloc();

private:
    constexpr static uint BUFFER_SIZE = 512 * 1024;

    struct CacheEntry
    {
        DynamicUniformBuffer buffer_{ BUFFER_SIZE };
        uint64 safeFrame_{};
        uint begin_{};
    };

    Array<CacheEntry> entries_;
};

}
