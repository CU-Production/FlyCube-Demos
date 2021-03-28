struct Particle
{
    float3 position;
    float lifetime;
    float3 scale;
    float3 velocity;
};

cbuffer emitterInfo
{
    float3 emitterPos;
    float2 particleLifeRange;
    float3 minVelocity;
    float3 maxVelocity;
    float3 gravity;
};

cbuffer global
{
    float time;
    float deltaTime;
};

StructuredBuffer<Particle> SpawnParticles;
RWStructuredBuffer<Particle> UpdateParticles;

[numthreads(32, 1, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{
    uint index = DTid.x;

    if (UpdateParticles[index].lifetime <= 0.0f)
    {
        UpdateParticles[index] = SpawnParticles[index];
    }
    else
    {
        UpdateParticles[index].position += UpdateParticles[index].velocity * deltaTime;
        UpdateParticles[index].velocity += gravity * deltaTime;
        UpdateParticles[index].lifetime -= deltaTime;
    }
}