#pragma once

#include "VkTypes.h"
#include "Types.h"

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
