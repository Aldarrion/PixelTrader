#pragma once

#include "Render/VkTypes.h"
#include "Common/Types.h"

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
