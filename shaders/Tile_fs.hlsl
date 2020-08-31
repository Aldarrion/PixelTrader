struct BindingUBO
{
    uint4 SRV[2]; // TODO use constant
};

#define BindingIdx(x) Bindings.SRV[x >> 2][x & 3]

Texture2D AllTextures[] : register(t0, space1);
ConstantBuffer<BindingUBO> Bindings : register(b0, space2);

SamplerState PointSampler : register(s0, space0);

struct ps_in
{
    float2 UV : TEXCOORD0;
    float4 Color : COLOR;
};

float4 main(ps_in input)
{
    float4 col = AllTextures[BindingIdx(0)].Sample(PointSampler, input.UV.xy);
    col *= input.Color;
    return col;
}
