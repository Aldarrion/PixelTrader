static float3 TriangleVerts[3] =
{
    float3(-0.5, -0.5, 0.5),
    float3(0.5, -0.5, 0.5),
    float3(0, 0.5, 0.5)
};

static float2 TriangleVertsPx[3] =
{
    float2(100, 500),
    float2(500, 500),
    float2(250, 100)
};

static float3 TriangleColors[3] = 
{
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(0, 0, 1)
};

static float2 TriangleUVs[3] = 
{
    float2(0, 1),
    float2(1, 1),
    float2(0.5, 0)
};

struct vs_out
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
    float2 UV : TEXCOORD0;
};

vs_out main(uint vertexId : SV_VertexID)
{
    float2 dimensions = float2(1280, 720);

    /*float2 pos = TriangleVertsPx[vertexId];

    o.Pos.x = (pos.x / dimensions.x) * 2 - 1;
    o.Pos.y = (pos.y / dimensions.y) * 2 - 1;
    o.Pos.z = 1;*/

    vs_out o;

    o.Pos = float4(TriangleVerts[vertexId], 1);
    o.Color = TriangleColors[vertexId];
    o.UV = TriangleUVs[vertexId];

    return o;
}
