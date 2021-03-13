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

cbuffer Settings
{
    float4 color;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
}

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
//    return color;
    return float4(input.col, 1.0f);
}
