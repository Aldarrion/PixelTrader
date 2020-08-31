
static const float3 CubeVerts[] =
{
    float3(-1.0, 1.0, -1.0),
    float3(1.0, 1.0, -1.0),
    float3(1.0, 1.0, 1.0),
    float3(-1.0, 1.0, 1.0),
    float3(-1.0, -1.0, -1.0),
    float3(1.0, -1.0, -1.0),
    float3(1.0, -1.0, 1.0),
    float3(-1.0, -1.0, 1.0),
    float3(-1.0, -1.0, 1.0),
    float3(-1.0, -1.0, -1.0),
    float3(-1.0, 1.0, -1.0),
    float3(-1.0, 1.0, 1.0),
    float3(1.0, -1.0, 1.0),
    float3(1.0, -1.0, -1.0),
    float3(1.0, 1.0, -1.0),
    float3(1.0, 1.0, 1.0),
    float3(-1.0, -1.0, -1.0),
    float3(1.0, -1.0, -1.0),
    float3(1.0, 1.0, -1.0),
    float3(-1.0, 1.0, -1.0),
    float3(-1.0, -1.0, 1.0),
    float3(1.0, -1.0, 1.0),
    float3(1.0, 1.0, 1.0),
    float3(-1.0, 1.0, 1.0),
};

static const float3 CubeNormals[] = 
{
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, 1.0),
};

static const float3 CubeColors[] = 
{
    float3(0.2, 1.0, 0.0),
    float3(0.2, 1.0, 0.0),
    float3(0.2, 1.0, 0.0),
    float3(1.0, 0.2, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 0.2, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.5, 0.7, 0.5),
    float3(0.5, 0.7, 0.5),
    float3(0.5, 0.7, 0.5),
    float3(0.1, 0.7, 0.5),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(0.1, 1.0, 0.0),
    float3(1.0, 0.2, 0.0),
};

static const uint CubeIndices[] = 
{
    3, 1, 0,
    2, 1, 3,

    6, 4, 5,
    7, 4, 6,

    11, 9, 8,
    10, 9, 11,

    14, 12, 13,
    15, 12, 14,

    19, 17, 16,
    18, 17, 19,

    22, 20, 21,
    23, 20, 22
};

struct vs_out
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
    float3 Normal : TEXCOORD0;
    float3 WorldPos : POSITION;
};

struct Vec
{
    float4x4    VP;
    float4      ViewPos;
};

ConstantBuffer<Vec> View : register(b1, space2);

vs_out main(uint vertID : SV_VERTEXID)
{
    float4x4 model = {
        10, 0, 0, 20,
        0, 10, 0, -30,
        0, 0, 10, 50,
        0, 0, 0, 1
    };

    vs_out o = vs_out(0);

    float4 pos = float4(CubeVerts[CubeIndices[vertID]], 1);
    o.Pos = pos * model * View.VP;
    o.WorldPos = (pos * model).xyz;

    o.Color = CubeColors[CubeIndices[vertID]];
    o.Normal = normalize(CubeNormals[CubeIndices[vertID]]);

    return o;
}