#include "DynamicUniformBuffer.h"

#include "Allocator.h"
#include "Render.h"
#include "vkr_Math.h"

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
    auto res = entries_[0].buffer_.Init();

    return res;
}

//------------------------------------------------------------------------------
DynamicUBOEntry DynamicUBOCache::BeginAlloc(uint size, void** data)
{
    uint minUboAlignment = (uint)g_Render->GetPhysDevProps().limits.minUniformBufferOffsetAlignment;
    vkr_assert(minUboAlignment > 0);
    size = Max(size, minUboAlignment);

    entries_[0].begin_ = Align(entries_[0].begin_, size);

    if (BUFFER_SIZE - entries_[0].begin_ < size)
    {
        if (entries_.Last().safeFrame_ <= g_Render->GetCurrentFrame())
        {
            entries_.Insert(0, entries_.Last());
            entries_.Remove(entries_.Count() - 1);
            entries_[0].begin_ = 0;
        }
        else
        {
            entries_.Insert(0, CacheEntry());
            entries_[0].buffer_.Init();
        }
    }

    DynamicUBOEntry result;
    result.buffer_ = entries_[0].buffer_.GetBuffer();
    result.dynOffset_ = entries_[0].begin_;
    result.size_ = size;

    *data = (uint8*)entries_[0].buffer_.Map() + entries_[0].begin_;

    entries_[0].safeFrame_ = g_Render->GetSafeFrame();
    entries_[0].begin_ += size;

    return result;
}

//------------------------------------------------------------------------------
void DynamicUBOCache::EndAlloc()
{
    entries_[0].buffer_.Unmap();
}

}
