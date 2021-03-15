struct VS_OUTPUT
{
    float4 pos: SV_POSITION;
    float3 col : COLOR;
};

static float4 vertPosArray[] =
{
    float4(-0.5f,  0.5f, 0.0f, 1.0f),
    float4( 0.5f,  0.5f, 0.0f, 1.0f),
    float4( 0.5f, -0.5f, 0.0f, 1.0f),
    float4(-0.5f, -0.5f, 0.0f, 1.0f)
};

static float3 vertColArray[] =
{
    float3( 1.0f,  0.0f, 0.0f),
    float3( 0.0f,  1.0f, 0.0f),
    float3( 0.0f,  0.0f, 1.0f),
    float3( 0.0f,  0.0f, 0.0f),
};

static uint3 vertIndicesArray[] =
{
    uint3(0, 1, 2),
    uint3(0, 2, 3),
};

[OutputTopology("triangle")]
[NumThreads(4, 1, 1)]
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

    if (gtid < numVertices)
    {
        outVerts[gtid].pos = vertPosArray[gtid];
        outVerts[gtid].col = vertColArray[gtid];
    }

    if (gtid < numPrimitives)
    {
        outIndices[gtid] = vertIndicesArray[gtid];
    }
}
