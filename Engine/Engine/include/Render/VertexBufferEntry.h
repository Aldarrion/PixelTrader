#pragma once

#include "Render/VkTypes.h"
#include "Common/Types.h"

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
