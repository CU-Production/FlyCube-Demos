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
    float3 objectVertex : TEXCOORD0;
    float3 vectorToSurface : TEXCOORD1;
};

VS_OUTPUT mainVS(VS_INPUT input)
{
    // VS_OUTPUT output;
    // output.pos = mul(float4(input.pos, 1.0f), WorldViewProj);
    // output.uv = input.uv;
    // return output;

    VS_OUTPUT o;

    // Vertex in object space this will be the starting point of raymarching
    o.objectVertex = input.pos;

    // Calculate vector from camera to vertex in world space
    float3 worldVertex = mul(float4(input.pos, 1.0f), World).xyz;
    o.vectorToSurface = worldVertex - CameraPos;

    o.vertex = mul(float4(input.pos, 1.0f), WorldViewProj);
    return o;
}
