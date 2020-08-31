struct Vec
{
    float4x4    Projection;
    float4      ViewPos;
};

ConstantBuffer<Vec> View : register(b1, space2);

struct ps_in
{
    float3 Color : COLOR;
    float3 Normal : TEXCOORD0;
    float3 FragPos : POSITION;
};

float4 main(ps_in input)
{
    float3 lightDir = normalize(float3(2, -1, 5));

    float4 outCol = float4(0);

    float3 toLight = -lightDir;

    float ambient = 0.03;
    float diffuse = saturate(dot(toLight, input.Normal));

    // Specular
    float specular = 0;
    if (diffuse > 0)
    {
        // Don't add specular for lights on the other side of the surface
        float exponent = 32;
        float3 toCamera = normalize(View.ViewPos.xyz - input.FragPos);
        float3 halfwayDir = normalize(toLight + toCamera);
        float d = saturate(dot(input.Normal, halfwayDir));
        specular = pow(d, exponent);
    }

    outCol = float4(input.Color * (diffuse + specular + ambient), 1);

    return outCol;
}