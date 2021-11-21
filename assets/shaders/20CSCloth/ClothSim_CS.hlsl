cbuffer constants
{
    float3 gravity;
    float RestLengthHoriz;
    float RestLengthVert;
    float RestLengthDiag;
};

const static float ParticleMass = 0.1;
const static float ParticleInvMass = 1.0 / 0.1;
// const static float SpringK = 2000.0;
const static float SpringK = 200.0;
// const static float DeltaT = 0.000005;
const static float DeltaT = 0.00167;
// const static float DampingConst = 0.1;
const static float DampingConst = 3.5;
// const static float DampingConst = 10.4;
const static uint2 ParticleCount = uint2(40, 40);

StructuredBuffer<float3> in_position;
StructuredBuffer<float3> in_velocity;

RWStructuredBuffer<float3> out_position;
RWStructuredBuffer<float3> out_velocity;

[numthreads(8, 8, 1)]
void mainCS(uint3 threadId : SV_DispatchthreadId, uint groupId : SV_GroupIndex, uint3 dispatchId : SV_GroupID)
{
    uint idx = threadId.y * ParticleCount.x + threadId.x;

    float3 p = in_position[idx];
    float3 v = in_velocity[idx];
    float3 r;

    // Start with gravitational acceleration and add the spring
    // forces from each neighbor
    float3 force = gravity * ParticleMass;

    // Particle above
    if (threadId.y < ParticleCount.y - 1) {
        r = in_position[idx + ParticleCount.x] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthVert);
    }
    // Below
    if (threadId.y > 0) {
        r = in_position[idx - ParticleCount.x] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthVert);
    }
    // Left
    if (threadId.x > 0) {
        r = in_position[idx - 1] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthHoriz);
    }
    // Right
    if (threadId.x < ParticleCount.x - 1) {
        r = in_position[idx + 1] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthHoriz);
    }

    // Diagonals
    // Upper-left
    if(threadId.x > 0 && threadId.y < ParticleCount.y - 1 ) {
        r = in_position[idx + ParticleCount.x - 1] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
    }
    // Upper-right
    if(threadId.x < ParticleCount.x - 1 && threadId.y < ParticleCount.y - 1) {
        r = in_position[idx + ParticleCount.x + 1] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
    }
    // lower -left
    if(threadId.x > 0 && threadId.y > 0 ) {
        r = in_position[idx - ParticleCount.x - 1] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
    }
    // lower-right
    if(threadId.x < ParticleCount.x - 1 && threadId.y > 0 ) {
        r = in_position[idx - ParticleCount.x + 1] - p;
        force += normalize(r) * SpringK * (length(r) - RestLengthDiag);
    }

    force += -DampingConst * v;

    // Apply simple Euler integrator
    float3 a = force * ParticleInvMass;
    out_position[idx] = p + v * DeltaT + 0.5 * a * DeltaT * DeltaT;
    out_velocity[idx] = v + a * DeltaT;

    // Pin a few of the top verts
    if(threadId.y == ParticleCount.y - 1 &&
          (threadId.x == 0 ||
           threadId.x == ParticleCount.x / 4 ||
           threadId.x == ParticleCount.x * 2 / 4 ||
           threadId.x == ParticleCount.x * 3 / 4 ||
           threadId.x == ParticleCount.x - 1)) {
        out_position[idx] = p;
        out_velocity[idx] = float3(0,0,0);
    }
}