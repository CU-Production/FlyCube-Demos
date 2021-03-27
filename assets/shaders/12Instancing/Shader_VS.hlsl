struct VS_INPUT
{
    float3 pos : POSITION;
    float3 col : COLOR;
    uint instanceID : SV_InstanceID;
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
    if (input.instanceID == 0)
    {
        output.pos.x = output.pos * 0.5f - 0.5f;
    }
    else
    {
        output.pos.x = output.pos * 0.5f + 0.5f;
        output.col = float3(1.0f, 1.0f, 1.0f) - output.col;
    }
    return output;
}

float4 mainPS(VS_OUTPUT input) : SV_TARGET
{
//    return color;
    return float4(input.col, 1.0f);
}
