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

cbuffer ConstantBuf
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
    float4x4 mvp;
};

struct Particle
{
    float3 position;
    float lifetime;
    float3 scale;
    float3 velocity;
};

StructuredBuffer<Particle> SpawnParticles;

VS_OUTPUT mainVS(VS_INPUT input)
{
    VS_OUTPUT output;
//     float4 worldPos = mul(float4(input.pos, 1.0f), model);
//     output.pos = mul(worldPos, mul(view, projection));;
    float3 pos = input.pos * SpawnParticles[input.instanceID].scale;
    pos += SpawnParticles[input.instanceID].position;
    output.pos = mul(float4(pos, 1.0f), mvp);
    output.col = input.col;
    return output;
}
