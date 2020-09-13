#if VS
struct vs_out
#else
struct ps_in
#endif
{
    #if VS
        float4 Pos : SV_POSITION;
    #endif
    float2 UV : TEXCOORD0;
    float4 Color : COLOR;
};
