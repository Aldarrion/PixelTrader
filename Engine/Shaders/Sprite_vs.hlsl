#include "SpriteCommon.h"
#include "shaderStructs/Common.h"

struct vertex
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR;
};

ConstantBuffer<SceneData>   Scene   : register(b1, space2);
ConstantBuffer<SpriteData>  Sprite  : register(b2, space2);

vs_out main(vertex vert)
{
    vs_out o;

    o.Pos = vert.Pos * Sprite.World * Scene.VP;
    o.UV = vert.UV;
    o.Color = vert.Color;

    return o;
}
