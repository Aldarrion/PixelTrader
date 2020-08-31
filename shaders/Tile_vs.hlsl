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

vs_out main(vertex vert)
{
    vs_out o;

    o.Pos = vert.Pos;
    o.UV = vert.UV;
    o.Color = vert.Color;

    return o;
}
