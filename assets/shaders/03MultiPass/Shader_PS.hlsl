struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D diffuseTexture;

SamplerState g_sampler;

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    //return float4(input.uv, 0.0f, 1.0f);
    float4 texColor = diffuseTexture.Sample(g_sampler, input.uv);
    return float4(texColor.rgb, 1.0f);
}
