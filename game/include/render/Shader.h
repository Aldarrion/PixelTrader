#pragma once

#include "render/VkTypes.h"
#include "common/Types.h"

namespace hs
{

//------------------------------------------------------------------------------
class Shader
{
public:
    VkShaderModule vkShader_;
    uint16 id_;
};

}
