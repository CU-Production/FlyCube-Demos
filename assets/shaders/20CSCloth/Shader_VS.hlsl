cbuffer Constants
{
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
    float3 CameraPos;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 vertex : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT o;

    o.vertex = mul(float4(input.pos, 1.0f), WorldViewProj);
    o.uv = input.uv;

    return o;
}
