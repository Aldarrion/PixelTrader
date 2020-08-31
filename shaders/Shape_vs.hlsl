struct vertex
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

struct vs_out
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
};

vs_out main(vertex vert)
{
    float2 dimensions = float2(1280, 720);

    vs_out o;

    o.Pos.x = (vert.Pos.x / dimensions.x) * 2 - 1;
    o.Pos.y = ((dimensions.y - vert.Pos.y) / dimensions.y) * 2 - 1;
    o.Pos.z = 1;
    o.Pos.w = 1;

    o.Color = vert.Color;

    return o;
}
