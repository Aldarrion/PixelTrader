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

float ToLinear(float srgbCol)
{
    float linearCol;
    if (srgbCol <= 0.04045)
        linearCol = srgbCol / 12.92;
    else
        linearCol = pow((srgbCol + 0.055) / 1.055, 2.4);

    return linearCol;
}

float ToSrgb(float linearCol)
{
    float srgbCol;
    if (linearCol <= 0.0031308)
        srgbCol = linearCol * 12.92;
    else
        srgbCol = 1.055 * pow(linearCol, 1.0 / 2.4) - 0.055;

    return srgbCol;
}

float4 ToLinear(float4 srgbCol)
{
    return float4(ToLinear(srgbCol.r), ToLinear(srgbCol.g), ToLinear(srgbCol.b), srgbCol.a);
}

float4 ToSrgb(float4 srgbCol)
{
    return float4(ToSrgb(srgbCol.r), ToSrgb(srgbCol.g), ToSrgb(srgbCol.b), srgbCol.a);
}

vs_out main(vertex vert)
{
    float2 dimensions = float2(1280, 720);

    vs_out o;

    o.Pos = vert.Pos * View.VP;

    o.Color = ToLinear(vert.Color);

    return o;
}
