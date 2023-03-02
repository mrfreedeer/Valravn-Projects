#include "ShaderCommon.hlsl"
RWStructuredBuffer<Cell> Grid : register(u0);

[numthreads(64, 1, 1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint numVerts = 0;
    uint stride = 0;
    Grid.GetDimensions(numVerts, stride);
    
    
    uint accessPos = threadId.x;
    if (accessPos <= numVerts)
    {
        Grid[accessPos].Density = 0;
        Grid[accessPos].Velocity = int3(0, 0, 0);
  
    }
    
    
}