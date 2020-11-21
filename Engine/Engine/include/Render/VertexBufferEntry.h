#pragma once

#include "render/VkTypes.h"
#include "common/Types.h"

namespace hs
{

class VertexBuffer;

//------------------------------------------------------------------------------
struct VertexBufferEntry
{
    VkBuffer buffer_{};
    uint offset_{};
};

}
