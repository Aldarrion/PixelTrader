#pragma once

#include "render/VkTypes.h"
#include "common/Types.h"

namespace hs
{

//------------------------------------------------------------------------------
struct DynamicUBOEntry
{
    VkBuffer buffer_{};
    uint dynOffset_{};
    uint size_{};
};

}
