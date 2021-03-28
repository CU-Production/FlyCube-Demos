struct VS_INPUT
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 col : COLOR;
};

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
    return float4(input.col, 1.0f);
}
