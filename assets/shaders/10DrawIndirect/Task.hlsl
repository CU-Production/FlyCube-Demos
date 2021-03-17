struct DrawIndexedIndirectCommand
{
    uint index_count;
    uint instance_count;
    uint first_index;
    int vertex_offset;
    uint first_instance;
};

RWStructuredBuffer<DrawIndexedIndirectCommand> Argument;

[numthreads(1, 1, 1)]
void MainCS(uint3 Gid  : SV_GroupID,          // current group index (dispatched by c++)
            uint3 DTid : SV_DispatchThreadID, // "global" thread id
            uint3 GTid : SV_GroupThreadID,    // current threadId in group / "local" threadId
            uint GI    : SV_GroupIndex)       // "flattened" index of a thread within a group)
{
    Argument[DTid.x].index_count = 3;
    Argument[DTid.y].instance_count = 1;
    Argument[DTid.z].first_index = 0;
    Argument[DTid.z].vertex_offset = 0;
    Argument[DTid.z].first_instance = 0;
}