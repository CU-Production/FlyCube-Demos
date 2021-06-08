// https://www.febucci.com/2019/05/fire-shader/

cbuffer computeUniformBlock
{
    float time;
}

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D noiseTexture;
Texture2D rampTexture;

SamplerState g_sampler;

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    const float3 RED = float3(1, 0, 0);
    const float3 YELLOW = float3(1, 1, 0);
    const float3 ORANGE = float3(1, 0.647, 0);

    float2 uv = input.uv;
    float x = 1.0f - rampTexture.Sample(g_sampler, uv.yx).x;

    uv.y -= time;

    float4 texColor = noiseTexture.Sample(g_sampler, uv);
    float y = texColor.x;

//     float tmp = step(y, x);
    float L1 = step(y, x) - step(y, x - 0.2f);

    float3 color1 = lerp(YELLOW, RED, L1);

    float3 L2 = step(y, x - 0.2f) - step(y, x - 0.4f);

    float3 color2 = lerp(color1, ORANGE, L2);

    return float4(color2, 1.0f);
}
