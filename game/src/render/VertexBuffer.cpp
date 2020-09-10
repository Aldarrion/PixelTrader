#include "render/VertexBuffer.h"

#include "render/Render.h"
#include "render/Allocator.h"

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

//------------------------------------------------------------------------------
void VertexBuffer::Free()
{
    vmaDestroyBuffer(g_Render->GetAllocator(), buffer_, allocation_);
}

}
