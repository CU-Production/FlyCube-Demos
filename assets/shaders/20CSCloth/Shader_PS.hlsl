struct VS_OUTPUT
{
    float4 vertex : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer Constants
{
    float4x4 WorldToObj;
};

Texture2D diffuseTexture;
SamplerState g_sampler;

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    float4 texColor = diffuseTexture.Sample(g_sampler, input.uv);
    return float4(texColor.rgb, 1.0f);
}
