#include "ShaderCommon.hlsl"


RWStructuredBuffer<HairData> HairInfo : register(u0);
RWStructuredBuffer<Cell> Grid : register(u3);



float3 GetVelocityForCell(int3 cell)
{
    int3 cellVelocity = 0.0f.xxx;
    int cellIndex = GetIndexForCoords(cell, GridDimensions);
    int unreasonableVal = -165465465;
    
    InterlockedCompareExchange(Grid[cellIndex].Velocity.x, unreasonableVal, unreasonableVal, cellVelocity.x);
    InterlockedCompareExchange(Grid[cellIndex].Velocity.y, unreasonableVal, unreasonableVal, cellVelocity.y);
    InterlockedCompareExchange(Grid[cellIndex].Velocity.z, unreasonableVal, unreasonableVal, cellVelocity.z);


    float3 velocityAsFloat = asfloat(cellVelocity);
    
    return asfloat(Grid[cellIndex].Velocity);

}


float GetDensityForCell(int3 cell)
{
    int cellDensity = 0;
    int cellIndex = GetIndexForCoords(cell, GridDimensions);
    
    int unreasonableVal = -165465465;
    
    
    InterlockedCompareExchange(Grid[cellIndex].Density, unreasonableVal, Grid[cellIndex].Density, cellDensity);


    //float3 velocityAsFloat = asfloat(cellVelocity);
    
    return asfloat(cellDensity);
}

void SetVelocityForCell(int3 cell, float3 velocity)
{
    int cellIndex = GetIndexForCoords(cell, GridDimensions);
    
    int3 velocityAsInt = asint(velocity);
    int3 originalValue;
    
    InterlockedExchange(Grid[cellIndex].Velocity.x, velocity.x, originalValue.x);
    InterlockedExchange(Grid[cellIndex].Velocity.y, velocity.y, originalValue.y);
    InterlockedExchange(Grid[cellIndex].Velocity.z, velocity.z, originalValue.z);
}

void AddVelocityToCell(int3 cell, float3 velocity)
{
    int cellIndex = GetIndexForCoords(cell, GridDimensions);
    
    float3 totalVelocity = velocity + GetVelocityForCell(cell);
    int3 velocityAsInt = asint(totalVelocity);
    int3 originalVel = int3(0, 0, 0);
    
    InterlockedExchange(Grid[cellIndex].Velocity.x, velocityAsInt.x, originalVel.x);
    InterlockedExchange(Grid[cellIndex].Velocity.y, velocityAsInt.y, originalVel.y);
    InterlockedExchange(Grid[cellIndex].Velocity.z, velocityAsInt.z, originalVel.z);
}

void AddDensityToCell(int3 cell, float density)
{
    int cellIndex = GetIndexForCoords(cell, GridDimensions);
    
    float totalDensity = GetDensityForCell(cell) + density;
    int densityAsInt = asint(totalDensity);
    int originalDensity = 0.0f;
    
    InterlockedExchange(Grid[cellIndex].Density, densityAsInt, originalDensity);
    
}

[numthreads(64, 1, 1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint numVerts = 0;
    uint stride = 0;
    HairInfo.GetDimensions(numVerts, stride);
    
    int hairStartInd = threadId.x * (HairSegmentCount + 1);
    //hairStartInd++; // The first hair position is fixed to the scalp, so it's not update, but it is used for the algorithm
    
    // Clearing out the Grid before any calculation
    for (uint cellIndex = 0; cellIndex < HairSegmentCount; cellIndex++)
    {
        uint cellAccessIndex = (threadId.x * HairSegmentCount) + cellIndex;
        Grid[cellAccessIndex].Density = 0;
        Grid[cellAccessIndex].Velocity = 0;
    }
    AllMemoryBarrierWithGroupSync();
    
    static float DeltaSqr = DeltaTime * DeltaTime;
    
    static float3 GravityForce = float3(0.0f, 0.0f, Gravity);
    static float3 ResultingForces = ExternalForces + GravityForce;
    
    HairInfo[hairStartInd].Position += float4(Displacement, 0.0f);
    
    [unroll(30)]
    for (uint segmentIndex = 1; segmentIndex < HairSegmentCount + 1; segmentIndex++)
    {
        uint accessPos = segmentIndex + hairStartInd;
        if (accessPos > numVerts)
            continue;
        
        float4 prevPosition = HairInfo[accessPos - 1].Position;
        
        float4 newPosition = HairInfo[accessPos].Position + (DeltaTime * HairInfo[accessPos].Velocity) + (DeltaSqr * float4(ResultingForces, 0.0f));
        float4 dispToCurrentPosition = normalize(newPosition - prevPosition) * SegmentLength;
        float4 CorrectedPosition = prevPosition + dispToCurrentPosition;
        float4 CorrectionVec = CorrectedPosition - newPosition;
        
        newPosition = CorrectedPosition;
        //newPosition = float4(HandleCollision(newPosition.xyz), 1.0f);
        
        float4 firstTerm = (newPosition - HairInfo[accessPos].Position);
        float4 secondTerm = DampingCoefficient * (-CorrectionVec);
        
        if (DeltaTime > EPSILON)
        {
            HairInfo[accessPos].Velocity = (firstTerm + secondTerm) / DeltaTime;
        }
        HairInfo[accessPos].Position = newPosition;
        
    }
    
    
    AllMemoryBarrierWithGroupSync(); // All must have update their velocities
    
    float3 modelPosition = ModelMatrix._14_24_34_44.xyz;
    float3 gridOrigin = modelPosition - float3(GridDimensions) * 0.5f;
    
    [unroll(30)]
    for (uint gridSegmentInd = 1; gridSegmentInd < HairSegmentCount + 1; gridSegmentInd++)
    {
        uint accessPos = gridSegmentInd + hairStartInd;
        HairData hairSegmentInfo = HairInfo[accessPos];
        
        int3 gridPosition = GetCoordsForPosition(hairSegmentInfo.Position.xyz, gridOrigin);
        float3 particleVelocity = HairInfo[accessPos].Velocity.xyz;
        float3 particlePosition = HairInfo[accessPos].Position.xyz - gridOrigin;
        
        for (int neighborX = gridPosition.x - 1; neighborX < gridPosition.x + 1; neighborX++)
        {
            for (int neighborY = gridPosition.y - 1; neighborY < gridPosition.y + 1; neighborY++)
            {
                for (int neighborZ = gridPosition.z - 1; neighborZ < gridPosition.z + 1; neighborZ++)
                {
                    int3 newGridPos = int3(neighborX, neighborY, neighborZ);
                    
                    float densityToAdd = 0.0f;
                    densityToAdd = (1.0f - abs(particlePosition.x - float(newGridPos.x)));
                    densityToAdd *= (1.0f - abs(particlePosition.y - float(newGridPos.y)));
                    densityToAdd *= (1.0f - abs(particlePosition.z - float(newGridPos.z)));
                    
                    densityToAdd = saturate(densityToAdd);
                   
                    AddDensityToCell(newGridPos, densityToAdd);
                    AddVelocityToCell(newGridPos, densityToAdd * particleVelocity);
  
                    
                }

            }

        }

    }
    
    AllMemoryBarrierWithGroupSync(); // All must have update their velocities
    
    float repulsion = 0.01f;
    
    [unroll(30)]
    for (uint velCorrectionIndex = 1; velCorrectionIndex < HairSegmentCount + 1; velCorrectionIndex++)
    {
        uint accessPos = velCorrectionIndex + hairStartInd;
        HairData hairSegmentInfo = HairInfo[accessPos];
        
        int3 gridPosition = GetCoordsForPosition(hairSegmentInfo.Position.xyz, gridOrigin);
        float3 particlePosition = HairInfo[accessPos].Position.xyz - gridOrigin;
        int gridAccessIndex = GetIndexForCoords(gridPosition, GridDimensions);
        
        int3 velAsInt = Grid[gridAccessIndex].Velocity;
        //float3 velAsFloat = asfloat(velAsInt);
        float3 velAsFloat = GetVelocityForCell(gridPosition);
        
        int densityAsInt = Grid[gridAccessIndex].Density;
        //float densityAsFloat = asfloat(densityAsInt);
        float densityAsFloat = GetDensityForCell(gridPosition);
        
        
        float3 gridVelocity = velAsFloat / densityAsFloat;
        
        float3 updatedVelocity = float3(0.0f, 0.0f, 0.0f);
        if ((densityAsFloat >= 0.05f) || (length(gridVelocity) < 1000.0f)) // This is an unreasanable speed. Every so often, some hairs disappear as they add an near infinite speed 1 frame
        {
            updatedVelocity = (1.0f - FrictionCoefficient) * HairInfo[accessPos].Velocity.xyz + FrictionCoefficient * gridVelocity;
        }
        else
        {
            updatedVelocity = HairInfo[accessPos].Velocity.xyz;
        }
        
        //updatedVelocity += repulsion * (gradient) / DeltaTime;
        
        float3 hairPosition = HairInfo[accessPos].Position.xyz;
        float3 updatedhairPosition = HandleCollision(hairPosition);
        //float3 velToClearFromPlane = HandlePlaneCollision(updatedhairPosition);
        
        //updatedVelocity += velToClearFromPlane;
        updatedVelocity += (updatedhairPosition - hairPosition) / DeltaTime;
        
        HairInfo[accessPos].Position = float4(updatedhairPosition, 1.0f);
        if (DeltaTime > EPSILON)
        {
            HairInfo[accessPos].Velocity = float4(updatedVelocity, 1.0f);
        }
        

    }
    
}