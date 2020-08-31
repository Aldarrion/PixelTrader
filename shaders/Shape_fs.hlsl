struct ps_in
{
    float3 Color : COLOR;
};

float4 main(ps_in input)
{
    return float4(input.Color, 1);
}
