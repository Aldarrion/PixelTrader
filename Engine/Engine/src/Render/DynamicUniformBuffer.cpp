#include "Render/DynamicUniformBuffer.h"

#include "Render/Allocator.h"
#include "Render/Render.h"
#include "Math/hs_Math.h"
#include "Common/Logging.h"

namespace hs
{

//------------------------------------------------------------------------------
DynamicUniformBuffer::DynamicUniformBuffer(uint size)
    : size_(size)
{
}

//------------------------------------------------------------------------------
RESULT DynamicUniformBuffer::Init()
{
    VkBufferCreateInfo buffInfo{};
    buffInfo.sType          = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffInfo.size           = size_;
    buffInfo.usage          = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffInfo.sharingMode    = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (VKR_FAILED(vmaCreateBuffer(g_Render->GetAllocator(), &buffInfo, &allocInfo, &buffer_, &allocation_, nullptr)))
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
void DynamicUniformBuffer::Free()
{
    if (buffer_ && allocation_)
        vmaDestroyBuffer(g_Render->GetAllocator(), buffer_, allocation_);
}

//------------------------------------------------------------------------------
void* DynamicUniformBuffer::Map()
{
    void* mapped{};
    if (VKR_FAILED(vmaMapMemory(g_Render->GetAllocator(), allocation_, &mapped)))
        return nullptr;

    return mapped;
}

//------------------------------------------------------------------------------
void DynamicUniformBuffer::Unmap()
{
    vmaUnmapMemory(g_Render->GetAllocator(), allocation_);
}

//------------------------------------------------------------------------------
VkBuffer DynamicUniformBuffer::GetBuffer() const
{
    return buffer_;
}

//------------------------------------------------------------------------------
uint DynamicUniformBuffer::GetSize() const
{
    return size_;
}

//------------------------------------------------------------------------------
RESULT DynamicUBOCache::Init()
{
    entries_.Add(CacheEntry());
    auto res = entries_.First().buffer_.Init();

    return res;
}

//------------------------------------------------------------------------------
DynamicUBOCache::~DynamicUBOCache()
{
    for (int i = 0; i < entries_.Count(); ++i)
    {
        entries_[i].buffer_.Free();
    }
}

//------------------------------------------------------------------------------
DynamicUBOEntry DynamicUBOCache::BeginAlloc(uint size, void** data)
{
    uint minUboAlignment = (uint)g_Render->GetPhysDevProps().limits.minUniformBufferOffsetAlignment;
    hs_assert(minUboAlignment > 0);
    size = Max(size, minUboAlignment);

    entries_.First().begin_ = Align(entries_.First().begin_, size);

    if (BUFFER_SIZE - entries_.First().begin_ < size)
    {
        if (entries_.Last().safeToUseFrame_ <= g_Render->GetCurrentFrame())
        {
            auto last = entries_.Last();
            entries_.Insert(0, last);
            entries_.RemoveLast();
            entries_.First().begin_ = 0;
        }
        else
        {
            entries_.Insert(0, CacheEntry());
            if (HS_FAILED(entries_.First().buffer_.Init()))
            {
                LOG_ERR("Failed to create new buffer entry");
                hs_assert(false);
                return {};
            }
        }
    }

    DynamicUBOEntry result;
    result.buffer_ = entries_.First().buffer_.GetBuffer();
    result.dynOffset_ = entries_.First().begin_;
    result.size_ = size;

    *data = (uint8*)entries_.First().buffer_.Map() + entries_.First().begin_;

    entries_.First().safeToUseFrame_ = g_Render->GetSafeFrame();
    entries_.First().begin_ += size;

    return result;
}

//------------------------------------------------------------------------------
void DynamicUBOCache::EndAlloc()
{
    entries_.First().buffer_.Unmap();
}

}
