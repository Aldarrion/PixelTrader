#pragma once
#include "Config.h"

namespace hs
{

//------------------------------------------------------------------------------
enum [[nodiscard]] RESULT : int
{
    R_FAIL = -1,
    R_OK = 0,
};

//------------------------------------------------------------------------------
enum PipelineStage
{
    PS_VERT,
    PS_FRAG,
    PS_COUNT
};

//------------------------------------------------------------------------------
enum class VkrPrimitiveTopology
{
    POINT_LIST = 0,
    LINE_LIST = 1,
    LINE_STRIP = 2,
    TRIANGLE_LIST = 3,
    TRIANGLE_STRIP = 4,
    TRIANGLE_FAN = 5,
    LINE_LIST_WITH_ADJACENCY = 6,
    LINE_STRIP_WITH_ADJACENCY = 7,
    TRIANGLE_LIST_WITH_ADJACENCY = 8,
    TRIANGLE_STRIP_WITH_ADJACENCY = 9,
    PATCH_LIST = 10
};

//------------------------------------------------------------------------------
enum class VkrCullMode
{
    Back,
    Front,
    None
};

}

