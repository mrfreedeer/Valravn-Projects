
RWStructuredBuffer<uint> buffer : register(u0);

[numthreads(64,1,1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    buffer[threadId.x] = buffer[threadId.x] + 1;
}