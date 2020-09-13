#ifdef __cplusplus
#pragma once

#include "hs_Math.h"

namespace hs
{
#endif

#ifndef __cplusplus
    #define Mat44 float4x4
    #define Vec4 float4

    struct BindingUBO
    {
        uint4 SRV[2]; // TODO use constant
    };

    #define BindingIdx(x) Bindings.SRV[x >> 2][x & 3]

    Texture2D BindlessTex2D[] : register(t0, space1);
    ConstantBuffer<BindingUBO> Bindings : register(b0, space2);

    #define GetTex2D(x) BindlessTex2D[BindingIdx(0)]
#endif

struct SceneCb
{
    Mat44   VP;
    Vec4    ViewPos;
};

#ifdef __cplusplus
}
#endif
