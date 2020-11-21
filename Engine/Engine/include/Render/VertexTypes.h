#pragma once

#include "render/Render.h"
#include "math/hs_Math.h"

namespace hs
{

//------------------------------------------------------------------------------
struct ShapeVertex
{
    Vec4 position_;
    uint color_;
    uint pad_[3];
};

//------------------------------------------------------------------------------
inline uint ShapeVertexLayout()
{
    static VkVertexInputAttributeDescription attributeDescriptions[2]{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_B8G8R8A8_UNORM;
    attributeDescriptions[1].offset = 16;

    static VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(ShapeVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = hs_arr_len(attributeDescriptions);
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    return g_Render->GetOrCreateVertexLayout(vertexInputInfo);
}

}
