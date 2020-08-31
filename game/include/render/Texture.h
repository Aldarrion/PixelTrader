#pragma once

#include "VkTypes.h"
#include "Enums.h"
#include "Types.h"

namespace hs
{

class Texture
{
public:
    enum class Type
    {
        TEX_2D,
        TEX_CUBE,
    };

    Texture(VkFormat format, VkExtent3D size, Type type);

    RESULT Allocate(void** data, const char* diagName = nullptr);
    void Free();

    VkImageView GetView() const;
    uint GetBindlessIndex() const;

private:
    VkImage         image_;
    VmaAllocation   allocation_;
    VkFormat        format_;
    VkExtent3D      size_;

    VkImageView     srv_;
    uint            bindlessIdx_;
    Type            type_;
};

}