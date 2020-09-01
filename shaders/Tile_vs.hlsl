struct vertex
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR;
};

struct vs_out
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
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
    vs_out o;

    o.Pos = vert.Pos * View.VP;
    o.UV = vert.UV;
    o.Color = vert.Color;

    return o;
}
