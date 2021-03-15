struct MyPayload
{
    uint myArbitraryData;
};

groupshared MyPayload payload;

[NumThreads(1, 1, 1)]
void mainAS(in uint3 gid : SV_GroupID)
{
    payload.myArbitraryData = gid.z; // means constant int 1
    DispatchMesh(1, 1, 1, payload);
}
