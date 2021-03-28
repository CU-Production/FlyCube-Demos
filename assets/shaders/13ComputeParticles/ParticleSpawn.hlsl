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

RWStructuredBuffer<Particle> SpawnParticles;

// Random number generation using pcg32i_random_t, using inc = 1. Our random state is a uint.
uint stepRNG(uint rngState)
{
    return rngState * 747796405 + 1;
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float stepAndOutputRNGFloat(inout uint rngState)
{
    // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
    rngState  = stepRNG(rngState);
    uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
    word      = (word >> 22) ^ word;
    return float(word) / 4294967295.0f;
}

[numthreads(32, 1, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{
    uint index = DTid.x;
    uint rngState = index + uint(time); // init seed

    float l = lerp(particleLifeRange.x, particleLifeRange.y, (stepAndOutputRNGFloat(rngState)));

    float3 v;
    v.x = lerp(minVelocity.x, maxVelocity.x, (stepAndOutputRNGFloat(rngState)));
    v.y = lerp(minVelocity.y, maxVelocity.y, (stepAndOutputRNGFloat(rngState)));
    v.z = lerp(minVelocity.z, maxVelocity.z, (stepAndOutputRNGFloat(rngState)));

    //SpawnParticles[index].position = p;
    SpawnParticles[index].position = emitterPos;
    SpawnParticles[index].lifetime = l;
    SpawnParticles[index].velocity = v;
    SpawnParticles[index].scale = float3(0.1f, 0.1f, 0.1f);

}