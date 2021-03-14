struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D screenTexture;

SamplerState g_sampler;

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    //return float4(input.uv, 0.0f, 1.0f);
    float4 texColor = screenTexture.Sample(g_sampler, input.uv);
    float grayScale = (texColor.r + texColor.g + texColor.b) / 3.0f;
    return float4(grayScale.xxx, 1.0f);
}
