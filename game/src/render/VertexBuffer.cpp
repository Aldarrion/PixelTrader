#include "render/VertexBuffer.h"

#include "render/Render.h"
#include "render/Allocator.h"
#include "common/Logging.h"

namespace hs
{

//------------------------------------------------------------------------------
VertexBuffer::VertexBuffer(uint size)
    : size_(size)
{
}

//------------------------------------------------------------------------------
RESULT VertexBuffer::Init()
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType          = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size           = size_;
    bufferInfo.usage          = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode    = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage         = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (VKR_FAILED(vmaCreateBuffer(g_Render->GetAllocator(), &bufferInfo, &allocInfo, &buffer_, &allocation_, nullptr)))
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
void* VertexBuffer::Map()
{
    void* mapped{};
    if (VKR_FAILED(vmaMapMemory(g_Render->GetAllocator(), allocation_, &mapped)))
        return nullptr;
    
    return mapped;
}

//------------------------------------------------------------------------------
void VertexBuffer::Unmap()
{
    vmaUnmapMemory(g_Render->GetAllocator(), allocation_);
}

//------------------------------------------------------------------------------
VkBuffer VertexBuffer::GetBuffer() const
{
    return buffer_;
}

uint VertexBuffer::GetSize() const
{
    return size_;
}

//------------------------------------------------------------------------------
void VertexBuffer::Free()
{
    vmaDestroyBuffer(g_Render->GetAllocator(), buffer_, allocation_);
}



//------------------------------------------------------------------------------
RESULT VertexBufferCache::Init()
{
    entries_.Add(CacheEntry());
    auto res = entries_.First().buffer_.Init();

    return res;
}

//------------------------------------------------------------------------------
VertexBufferCache::~VertexBufferCache()
{
    for (int i = 0; i < entries_.Count(); ++i)
        entries_[i].buffer_.Free();
    entries_.Clear();
}

//------------------------------------------------------------------------------
VertexBufferEntry VertexBufferCache::BeginAlloc(uint size, uint align, void** data)
{
    entries_.First().begin_ = Align(entries_.First().begin_, align);

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

    VertexBufferEntry result;
    result.buffer_ = entries_.First().buffer_.GetBuffer();
    result.offset_ = entries_.First().begin_;

    *data = (uint8*)entries_.First().buffer_.Map() + entries_.First().begin_;

    entries_.First().safeToUseFrame_ = g_Render->GetSafeFrame();
    entries_.First().begin_ += size;

    return result;
}

//------------------------------------------------------------------------------
void VertexBufferCache::EndAlloc()
{
    entries_.First().buffer_.Unmap();
}


}
