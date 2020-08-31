#include "render/Buffer.h"

#include "render/Render.h"
#include "render/Allocator.h"

namespace hs
{

//------------------------------------------------------------------------------
StagingBuffer::StagingBuffer(uint size)
    : size_(size)
{
}

//------------------------------------------------------------------------------
RESULT StagingBuffer::Allocate(void* data)
{
    // Create the buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType        = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size         = size_;
    bufferInfo.usage        = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    if (VKR_FAILED(vmaCreateBuffer(g_Render->GetAllocator(), &bufferInfo, &allocInfo, &buffer_, &allocation_, nullptr)))
        return R_FAIL;

    if (data)
    {
        void* mapped;
        if (VKR_FAILED(vmaMapMemory(g_Render->GetAllocator(), allocation_, &mapped)))
            return R_FAIL;

        memcpy(mapped, data, size_);

        vmaUnmapMemory(g_Render->GetAllocator(), allocation_);
    }

    return R_OK;
}

//------------------------------------------------------------------------------
VkBuffer StagingBuffer::GetBuffer() const
{
    return buffer_;
}

}

