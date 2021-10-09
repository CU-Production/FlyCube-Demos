RWTexture3D<float4> uav;
Texture2D volumeSlicesTex;

[numthreads(4, 4, 4)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{
    int g_x = DTid.z % 16;
    int g_y = DTid.z / 16;
//     int3 sampleLocation = int3(g_x * 128 + DTid.x, g_y * 128 + DTid.y, 0);
    int3 sampleLocation = int3(g_x * 128 + DTid.x, g_y * 128 + ( 128 - DTid.y ), 0); // FlipY
    uav[DTid] = float4(volumeSlicesTex.Load(sampleLocation).xyz, 1.0f);
}