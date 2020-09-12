#pragma once

#include "render/DynamicUniformBufferEntry.h"

#include "common/Types.h"
#include "render/VkTypes.h"
#include "common/Enums.h"
#include "containers/Array.h"

namespace hs
{

//------------------------------------------------------------------------------
class DynamicUniformBuffer
{
public:
    DynamicUniformBuffer(uint size);

    RESULT Init();
    void Free();

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
    ~DynamicUBOCache();

    RESULT Init();

    DynamicUBOEntry BeginAlloc(uint size, void** data);
    void EndAlloc();

private:
    constexpr static uint BUFFER_SIZE = 512 * 1024;

    struct CacheEntry
    {
        DynamicUniformBuffer buffer_{ BUFFER_SIZE };
        uint64 safeToUseFrame_{};
        uint begin_{};
    };

    Array<CacheEntry> entries_;
};

}
