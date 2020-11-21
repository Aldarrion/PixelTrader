#include "Util.h"

struct vertex
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

struct vs_out
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

struct Vec
{
    float4x4    VP;
    float4      ViewPos;
};

ConstantBuffer<Vec> View : register(b1, space2);

vs_out main(vertex vert)
{
    float2 dimensions = float2(1280, 720);

    vs_out o;

    o.Pos = vert.Pos * View.VP;

    o.Color = ToLinear(vert.Color);

    return o;
}
