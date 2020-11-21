struct BindingUBO
{
    uint4 SRV[2]; // TODO use constant
};

#define BindingIdx(x) Bindings.SRV[x >> 2][x & 3]

SamplerState PointSampler : register(s0, space0);
Texture2D AllTextures[] : register(t0, space1);
ConstantBuffer<BindingUBO> Bindings : register(b0, space2);

struct ps_in
{
    float4 Pos : SV_POSITION;
    float3 Color : COLOR;
    float2 UV : TEXCOORD0;
};

float4 main(ps_in input)
{
    float4 outColor = float4(input.Color, 1);
    outColor = outColor * AllTextures[BindingIdx(1)].Sample(PointSampler, input.UV);

    return outColor;
}
