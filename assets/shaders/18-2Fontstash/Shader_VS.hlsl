cbuffer vertexBuffer
{
    float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos, 0.0f, 1.0f));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
