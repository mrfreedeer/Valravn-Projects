#include "Constants.hlsl"

RWStructuredBuffer<HairData> HairInfo : register(u0);

[numthreads(64, 1, 1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint numVerts = 0;
    uint stride = 0;
    HairInfo.GetDimensions(numVerts, stride);
    
    
    uint accessPos = threadId.x * 2;
    if (accessPos + 1 <= numVerts)
    {
        float4 position = HairInfo[accessPos].Position;
        float4 nextHairPosition = HairInfo[accessPos + 1].Position;
    
        position = mul(ModelMatrix, position);
        nextHairPosition = mul(ModelMatrix, nextHairPosition);
    
        HairInfo[accessPos].Position = position;
        HairInfo[accessPos + 1].Position = nextHairPosition;
  
    }
    
    
}