struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 col : COLOR;
};

[OutputTopology("triangle")]
[NumThreads(1, 1, 1)]
void mainMS(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 outIndices[2],
    out vertices VS_OUTPUT outVerts[4]
)
{
    const uint numVertices = 4;
    const uint numPrimitives = 2;

    SetMeshOutputCounts(numVertices, numPrimitives);

    outVerts[0].pos = float4(-0.5f,  0.5f, 0.0f, 1.0f);
    outVerts[0].col = float3( 1.0f,  0.0f, 0.0f);

    outVerts[1].pos = float4( 0.5f,  0.5f, 0.0f, 1.0f);
    outVerts[1].col = float3( 0.0f,  1.0f, 0.0f);

    outVerts[2].pos = float4( 0.5f, -0.5f, 0.0f, 1.0f);
    outVerts[2].col = float3( 0.0f,  0.0f, 1.0f);

    outVerts[3].pos = float4(-0.5f, -0.5f, 0.0f, 1.0f);
    outVerts[3].col = float3( 0.0f,  0.0f, 0.0f);

    outIndices[0] = uint3(0, 1, 2);
    outIndices[1] = uint3(0, 2, 3);
}
