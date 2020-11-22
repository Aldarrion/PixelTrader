#include "Render/Texture.h"

#include "Render/Render.h"
#include "Render/Allocator.h"
#include "Render/Buffer.h"
#include "Common/Logging.h"

#include "Render/hs_Image.h"

namespace hs
{

//------------------------------------------------------------------------------
uint Texture::GetWidth() const
{
    return size_.width;
}

//------------------------------------------------------------------------------
uint Texture::GetHeight() const
{
    return size_.height;
}

//------------------------------------------------------------------------------
uint Texture::GetDepth() const
{
    return size_.depth;
}

//------------------------------------------------------------------------------
RESULT Texture::CreateTex2D(const char* file, const char* name, Texture** tex)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        Log(LogLevel::Error, "Failed to read texture file: %s", file);
        return R_FAIL;
    }

    *tex = new Texture(VK_FORMAT_R8G8B8A8_SRGB, VkExtent3D{ (uint)texWidth, (uint)texHeight, 1 }, Texture::Type::TEX_2D);

    auto texAllocRes = (*tex)->Allocate((void**)&pixels, name);
    stbi_image_free(pixels);

    if (HS_FAILED(texAllocRes))
        return R_FAIL;

    return R_OK;
}

//------------------------------------------------------------------------------
Texture::Texture(VkFormat format, VkExtent3D size, Type type)
    : format_(format)
    , size_(size)
    , type_(type)
{
}

//------------------------------------------------------------------------------
VkImageView Texture::GetView() const
{
    return srv_;
}

//------------------------------------------------------------------------------
uint Texture::GetBindlessIndex() const
{
    return bindlessIdx_;
}

//------------------------------------------------------------------------------
RESULT Texture::Allocate(void** data, const char* diagName)
{
    VkImageLayout initLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageCreateInfo imgInfo{};
    VkImageViewCreateInfo imgViewInfo{};

    if (type_ == Type::TEX_CUBE)
    {
        imgInfo.flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imgInfo.arrayLayers = 6;

        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }
    else
    {
        imgInfo.arrayLayers = 1;

        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    }

    imgInfo.sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imgInfo.imageType       = VK_IMAGE_TYPE_2D;
    imgInfo.format          = format_;
    imgInfo.extent          = size_;
    imgInfo.mipLevels       = 1; // TODO implement mipmapping
    imgInfo.samples         = VK_SAMPLE_COUNT_1_BIT;
    imgInfo.tiling          = VK_IMAGE_TILING_OPTIMAL;
    imgInfo.usage           = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imgInfo.sharingMode     = VK_SHARING_MODE_EXCLUSIVE;
    imgInfo.initialLayout   = initLayout;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage         = VMA_MEMORY_USAGE_GPU_ONLY;

    if (VKR_FAILED(vmaCreateImage(g_Render->GetAllocator(), &imgInfo, &allocInfo, &image_, &allocation_, nullptr)))
        return R_FAIL;

    if (diagName)
    {
        if (VKR_FAILED(SetDiagName(g_Render->GetDevice(), (uint64)image_, VK_OBJECT_TYPE_IMAGE, diagName)))
            Log(LogLevel::Error, "Could not set diag name to a texture %s", diagName);
    }

    VkImageSubresourceRange allSubres{};
    allSubres.aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
    allSubres.baseMipLevel     = 0;
    allSubres.levelCount       = 1;
    allSubres.baseArrayLayer   = 0;
    allSubres.layerCount       = 1;

    if (data)
    {
        g_Render->TransitionBarrier(
            image_, allSubres,
            0, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        // For cube and cube array image views, the layers of the image view starting at
        // baseArrayLayer correspond to faces in the order +X, -X, +Y, -Y, +Z, -Z.
        for (uint i = 0; i < imgInfo.arrayLayers; ++i)
        {
            uint buffSize = size_.width * size_.height * 4;
            TempStagingBuffer staging(buffSize);
            if (FAILED(staging.Allocate(data[i])))
                return R_FAIL;

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = i;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = size_;

            vkCmdCopyBufferToImage(g_Render->CmdBuff(), staging.GetBuffer(), image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        g_Render->TransitionBarrier(
            image_, allSubres,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
    }

    imgViewInfo.sType               = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imgViewInfo.image               = image_;
    imgViewInfo.format              = format_;
    imgViewInfo.subresourceRange    = allSubres;

    // TODO do we need to keep the image view alive when it is copied to the bindless array?
    vkCreateImageView(g_Render->GetDevice(), &imgViewInfo, nullptr, &srv_);

    bindlessIdx_ = g_Render->AddBindlessTexture(srv_);

    return R_OK;
}

//------------------------------------------------------------------------------
void Texture::Free()
{
    vkDestroyImageView(g_Render->GetDevice(), srv_, nullptr);
    vmaDestroyImage(g_Render->GetAllocator(), image_, allocation_);
}

}
