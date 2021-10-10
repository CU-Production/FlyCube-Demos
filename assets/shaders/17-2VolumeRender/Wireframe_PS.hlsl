// Allowed floating point inaccuracy
#define LINEWIDTH 0.015f

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 localPos : TEXCOORD0;
};

cbuffer settings
{
    float4 wireColor;
}

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    float4 resultColor = wireColor;

    if (abs(input.localPos.x) - 0.5f < -LINEWIDTH && abs(input.localPos.y) - 0.5f < -LINEWIDTH)
        resultColor.a = 0.0f;
    if (abs(input.localPos.y) - 0.5f < -LINEWIDTH && abs(input.localPos.z) - 0.5f < -LINEWIDTH)
        resultColor.a = 0.0f;
    if (abs(input.localPos.z) - 0.5f < -LINEWIDTH && abs(input.localPos.x) - 0.5f < -LINEWIDTH)
        resultColor.a = 0.0f;
    return resultColor;
}
