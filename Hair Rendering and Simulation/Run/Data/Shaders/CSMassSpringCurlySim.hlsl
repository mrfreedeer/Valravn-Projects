#include "ShaderCommon.hlsl"
#define MAX_HAIR_SEGMENTS 30

RWStructuredBuffer<HairData> HairInfo : register(u0);
RWStructuredBuffer<HairData> HairInfoPrevFrame : register(u1);

// Helper function to make it easier to get the correct position according to useHalf or isVirtual parameters
float4 GetPositionAtIndex(uint index, bool useHalfPosition, bool isVirtual)
{
    float4 position = HairInfo[index].Position;
    float4 halfPosition = (HairInfo[index].Position + HairInfoPrevFrame[index].Position) * 0.5f;
     
    if (useHalfPosition)
        return halfPosition;
    
    return position;
}


// Curly hair has a specific spring configuration. Restitution forces are calculated here
void CalculateRestitutionForces(uint startIndex, inout float3 forces[MAX_HAIR_SEGMENTS], bool useHalfPosition)
{
    [unroll(MAX_HAIR_SEGMENTS)]
    for (uint hairSegmentIndex = 0; hairSegmentIndex < HairSegmentCount; hairSegmentIndex++)
    {
        int accessIndex = startIndex + hairSegmentIndex;
        int nextAccessIndex = accessIndex + 1;
        uint bendAccessIndex = accessIndex + 2;
        uint torsionAccessIndex = accessIndex + 3;
        
        float3 positionOne = GetPositionAtIndex(accessIndex, useHalfPosition, false).xyz;
        float3 positionTwo = GetPositionAtIndex(nextAccessIndex, useHalfPosition, false).xyz;
                
        float3 displacement = positionTwo - positionOne;
        float3 currentVelocity = HairInfo[accessIndex].Velocity.xyz;
        float3 velocityDeltaEdge = HairInfo[nextAccessIndex].Velocity.xyz - currentVelocity;
        
        float3 resultingForce = GetSpringForce(displacement, velocityDeltaEdge, EdgeStiffness, SegmentLength);
        forces[hairSegmentIndex] += resultingForce * 0.5f;
        forces[hairSegmentIndex + 1] += -resultingForce * 0.5f;
                
        bool calculateBend = (hairSegmentIndex + 2) <= HairSegmentCount;
        bool calculateTorsion = (hairSegmentIndex + 3) <= HairSegmentCount;
        
        
        // If there exists another position that I can connect the bend spring to, calculate the forces to it
        if (calculateBend)
        {
            float3 bendPositionTwo = GetPositionAtIndex(bendAccessIndex, useHalfPosition, false).xyz;
            float3 displacementBend = (bendPositionTwo - positionOne).xyz;
            float3 velocityDeltaBend = (HairInfo[bendAccessIndex].Velocity.xyz - currentVelocity);
            
            float3 bendSpringForce = GetSpringForce(displacementBend, velocityDeltaBend, BendStiffness, BendInitialLength);
            forces[hairSegmentIndex] += bendSpringForce * 0.5f;
            forces[hairSegmentIndex + 2] += -bendSpringForce * 0.5f;
        }
        
         // If there exists another position that I can connect the torsion spring to, calculate the forces to it
        if (calculateTorsion)
        {
            float3 torsionPositionTwo = GetPositionAtIndex(torsionAccessIndex, useHalfPosition, false).xyz;
            float3 displacementTorsion = (torsionPositionTwo - positionOne);
            float3 velocityDeltaTorsion = (HairInfo[torsionAccessIndex].Velocity.xyz - currentVelocity);
            
            float3 torsionSpringForce = GetSpringForce(displacementTorsion, velocityDeltaTorsion, TorsionStiffness, TorsionInitialLength);
            forces[hairSegmentIndex] += torsionSpringForce * 0.5f;
            forces[hairSegmentIndex + 3] += -torsionSpringForce * 0.5f;
        }
        
    }

}

// Zero-out array
void ClearArray(inout float3 forces[MAX_HAIR_SEGMENTS])
{
    for (uint forceIndex = 0; forceIndex < MAX_HAIR_SEGMENTS; forceIndex++)
    {
        forces[forceIndex] = float3(0.0f, 0.0f, 0.0f);
    }

}


[numthreads(64, 1, 1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint numVerts = 0;
    uint stride = 0;
    HairInfo.GetDimensions(numVerts, stride);
    
    uint hairStartInd = threadId.x * (HairSegmentCount + 1);
    
    static float3 GravityForce = float3(0.0f, 0.0f, Gravity);
    static float3 ResultingForces = ExternalForces + GravityForce;
    
    uint totalPositions = HairSegmentCount + 1;
    
    float3 forces[MAX_HAIR_SEGMENTS];
    
    // Ensure array is zeros
    ClearArray(forces);
            
    // Get first pass forces
    CalculateRestitutionForces(hairStartInd, forces, false);
    
    HairInfoPrevFrame[hairStartInd] = HairInfo[hairStartInd];
    [unroll(MAX_HAIR_SEGMENTS)]
    for (uint velocityIndex = 1; velocityIndex < HairSegmentCount + 1; velocityIndex++)
    {
        uint velAccessIndex = velocityIndex + hairStartInd;
        forces[velocityIndex] += ResultingForces;
        float4 prevPosition = HairInfo[velAccessIndex - 1].Position;
        
        float3 halfVelocity = HairInfo[velAccessIndex].Velocity.xyz + DeltaTime * 0.5f * forces[velocityIndex] / Mass;
        float4 limitedPosition = ApplyStrainLimiting(prevPosition, HairInfo[velAccessIndex].Position, halfVelocity, DeltaTime);
        
        HairInfoPrevFrame[velAccessIndex] = HairInfo[velAccessIndex];
        if (DeltaTime >= EPSILON)
        {
            float4 velocityCorrection = (limitedPosition - HairInfo[velAccessIndex].Position) / (DeltaTime);
            halfVelocity += velocityCorrection.xyz * StrainLimitingCoefficient;
        }
        
        
        float4 updatedPosition = float4(HairInfo[velAccessIndex].Position.xyz + DeltaTime * halfVelocity, 1.0f);
        float4 correctedPosition = float4(HandleCollision(updatedPosition.xyz), 1.0f);
        
        //HairInfo[velAccessIndex].Velocity = float4(halfVelocity, 1.0f);
        if (DeltaTime >= EPSILON)
        {
            
            float4 updatedVelocity = (correctedPosition - updatedPosition) / DeltaTime;
            HairInfo[velAccessIndex].Velocity += updatedVelocity;

        }
        
        HairInfo[velAccessIndex].Position = correctedPosition;
    }
    
    // Clear out forces 
    ClearArray(forces);
    
    // Get updated forces, second pass
    CalculateRestitutionForces(hairStartInd, forces, true);

    [unroll(MAX_HAIR_SEGMENTS)]
    for (uint velUpdateIndex = 1; velUpdateIndex < HairSegmentCount + 1; velUpdateIndex++)
    {
        uint velAccessIndexSecPass = velUpdateIndex + hairStartInd;
        forces[velUpdateIndex] += ResultingForces;
        
        float3 halfVelocity = HairInfo[velAccessIndexSecPass].Velocity.xyz + DeltaTime * 0.5f * forces[velUpdateIndex] / Mass;
        float3 updatedVel = (2.0f * halfVelocity) - HairInfo[velAccessIndexSecPass].Velocity.xyz;
        
        // Curly hair has much more energy to it, so friction helps the hair move yet settle down over time
        HairInfo[velAccessIndexSecPass].Velocity = float4(updatedVel, 1.0f) * (1.0f - FrictionCoefficient * FrictionCoefficient); 
    }
    
}