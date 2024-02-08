#include "ShaderCommon.hlsl"
#define MAX_HAIR_SEGMENTS 30

RWStructuredBuffer<HairData> HairInfo : register(u0);
RWStructuredBuffer<HairData> HairInfoPrevFrame : register(u1);
RWStructuredBuffer<HairData> VirtualHairInfo : register(u2);
RWStructuredBuffer<Cell> Grid : register(u3);


// Helper function to make it easier to get the correct position according to useHalf or isVirtual parameters
float4 GetPositionAtIndex(uint index, bool useHalfPosition, bool isVirtual)
{
    float4 position = HairInfo[index].Position;
    float4 halfPosition = (HairInfo[index].Position + HairInfoPrevFrame[index].Position) * 0.5f;
    
    float4 virtualPosition = VirtualHairInfo[index].Position;
    
    if (isVirtual)
        return virtualPosition;
    
    if (useHalfPosition)
        return halfPosition;
    
    return position;
}


// Curly hair has a specific spring configuration. Restitution forces are calculated here
void CalculateRestitutionForces(uint startIndex, inout float3 forces[MAX_HAIR_SEGMENTS], inout float3 virtualForces[MAX_HAIR_SEGMENTS], bool useHalfPosition)
{
    [unroll(MAX_HAIR_SEGMENTS)]
    for (uint hairSegmentIndex = 0; hairSegmentIndex < HairSegmentCount; hairSegmentIndex++)
    {
        int accessIndex = startIndex + hairSegmentIndex;
        int nextAccessIndex = accessIndex + 1;
        uint bendAccessIndex = accessIndex + 2;
        
        float3 positionOne = GetPositionAtIndex(accessIndex, useHalfPosition, false).xyz;
        float3 positionTwo = GetPositionAtIndex(nextAccessIndex, useHalfPosition, false).xyz;
                
        float3 displacement = positionTwo - positionOne;
        float3 currentVelocity = HairInfo[accessIndex].Velocity.xyz;
        float3 velocityDeltaEdge = HairInfo[nextAccessIndex].Velocity.xyz - currentVelocity;
        
        float3 resultingForce = GetSpringForce(displacement, velocityDeltaEdge, EdgeStiffness, SegmentLength);
        forces[hairSegmentIndex] += resultingForce * 0.5f;
        forces[hairSegmentIndex + 1] += -resultingForce * 0.5f;
                
        bool calculateBend = (hairSegmentIndex + 2) <= HairSegmentCount;
        
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
        
        /**
           CALCULATING FORCES USING VIRTUAL HAIR PARTICLES
           THERE'S ONE LESS THAN THE HAIR SEGMENTS
        **/
        
        uint maxVirtualParticles = HairSegmentCount - 1;
        if (hairSegmentIndex < maxVirtualParticles)
        {
            uint previousHairInd = accessIndex - 1;
            
            bool calculateEdgeNextPart = (hairSegmentIndex) < maxVirtualParticles;
            bool calculateTorsion = (HairSegmentCount - 1) > 0;
            
            float4 particlePosition = GetPositionAtIndex(accessIndex, useHalfPosition, true);
            float4 nextParticlePosition = GetPositionAtIndex(nextAccessIndex, useHalfPosition, true);
            float4 previousHairPosition = GetPositionAtIndex(previousHairInd, useHalfPosition, false);
            
            float3 currentHairPosition = positionOne;
            float3 nextHairPosition = positionTwo;
            
            float3 displacementToCurrent = (particlePosition.xyz - currentHairPosition);
            float3 velocityDeltaCurrent = VirtualHairInfo[accessIndex].Velocity.xyz - currentVelocity;
            
            // Restitution force to current hair position
            float3 forceEdgeCurrentHair = GetSpringForce(displacementToCurrent, velocityDeltaCurrent, EdgeStiffness, SegmentLength);
            forces[hairSegmentIndex] += forceEdgeCurrentHair * 0.5f;
            virtualForces[hairSegmentIndex] += -forceEdgeCurrentHair * 0.5f;
            
            float3 dispToNextHair = (nextHairPosition - particlePosition.xyz);
            float3 velocityDeltaNextHair = (HairInfo[nextAccessIndex].Velocity - VirtualHairInfo[accessIndex].Velocity).xyz;
            
            float3 forceEdgeNextHair = GetSpringForce(dispToNextHair, velocityDeltaNextHair, EdgeStiffness, SegmentLength);
            virtualForces[hairSegmentIndex] += forceEdgeNextHair * 0.5f;
            forces[hairSegmentIndex + 1] += -forceEdgeNextHair * 0.5f;
            
            if (calculateEdgeNextPart)
            {
                float3 dispNextVirtual = (nextParticlePosition - particlePosition).xyz;
                float3 velocityDeltaNextVirtual = (VirtualHairInfo[nextAccessIndex].Velocity - VirtualHairInfo[accessIndex].Velocity).xyz;
                
                float3 forcesNextVirtual = GetSpringForce(dispNextVirtual, velocityDeltaNextVirtual, BendStiffness, BendInitialLength);
                
                virtualForces[hairSegmentIndex] += forcesNextVirtual * 0.5f;
                virtualForces[hairSegmentIndex + 1] += -forcesNextVirtual * 0.5f;
                
            }
            
            
            if (calculateTorsion)
            {
                float3 dispToPrevHair = (particlePosition - previousHairPosition).xyz;
                float3 velocityDeltaTorsion = (VirtualHairInfo[accessIndex].Velocity - HairInfo[previousHairInd].Velocity).xyz;
                float3 torsionForce = GetSpringForce(dispToPrevHair, velocityDeltaTorsion, TorsionStiffness, TorsionInitialLength);
                
                forces[hairSegmentIndex - 1] += torsionForce * 0.5f;
                virtualForces[hairSegmentIndex] += -torsionForce * 0.5f;

            }
            
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
    float3 virtualForces[MAX_HAIR_SEGMENTS];
   
    // Ensure array is zeros
    ClearArray(forces);
    ClearArray(virtualForces);
            
    // Get first pass forces
    HairInfo[hairStartInd].Position += float4(Displacement, 0.0f);
    HairInfoPrevFrame[hairStartInd] = HairInfo[hairStartInd];
    
    CalculateRestitutionForces(hairStartInd, forces, virtualForces, false);
    
    
    
    [unroll(MAX_HAIR_SEGMENTS)]
    for (uint velocityIndex = 0; velocityIndex < HairSegmentCount + 1; velocityIndex++)
    {
        uint velAccessIndex = velocityIndex + hairStartInd;
        if (velocityIndex > 0) // All virtual velocities must be updated, but real hair only after root. But less for-loops the better!
        {
            forces[velocityIndex] += ResultingForces;
        
            float4 previousPos = HairInfo[velAccessIndex - 1].Position;
            float3 halfVelocity = HairInfo[velAccessIndex].Velocity.xyz + DeltaTime * 0.5f * forces[velocityIndex] / Mass;
            float4 limitedPosition = ApplyStrainLimiting(previousPos, HairInfo[velAccessIndex].Position, halfVelocity, DeltaTime);
            
            HairInfoPrevFrame[velAccessIndex] = HairInfo[velAccessIndex];
            if (DeltaTime >= EPSILON)
            {
                float3 velocityUpdate = (limitedPosition - HairInfo[velAccessIndex].Position).xyz / (DeltaTime);
                halfVelocity += velocityUpdate * StrainLimitingCoefficient;

            }
            
            float4 updatedPosition = float4(HairInfo[velAccessIndex].Position.xyz + DeltaTime * halfVelocity, 1.0f);
            float4 correctedPosition = float4(HandleCollision(updatedPosition.xyz), 1.0f);
            
            if (DeltaTime >= EPSILON)
            {
                float4 collisionVelUpdate = (correctedPosition - updatedPosition) / DeltaTime;
                HairInfo[velAccessIndex].Velocity += collisionVelUpdate;

            }
            
            HairInfo[velAccessIndex].Position = correctedPosition;
        }
        
        virtualForces[velocityIndex] += ResultingForces;
        
        // Virtual particles must also be updated
        float3 halfVelocityVirtual = VirtualHairInfo[velAccessIndex].Velocity.xyz + DeltaTime * 0.5f * virtualForces[velocityIndex] / Mass;
        
        float4 prevPosForVirtual = HairInfo[velAccessIndex].Position;
        float4 limitedVirtual = ApplyStrainLimiting(prevPosForVirtual, VirtualHairInfo[velAccessIndex].Position, halfVelocityVirtual, DeltaTime);
        
        if (DeltaTime >= EPSILON)
        {
            float4 virtualVelocityUpdate = (limitedVirtual - VirtualHairInfo[velAccessIndex].Position) / (DeltaTime);
            halfVelocityVirtual += (virtualVelocityUpdate * StrainLimitingCoefficient).xyz;
        }
        
        float4 updatedVirtualPos = float4(VirtualHairInfo[velAccessIndex].Position.xyz + DeltaTime * halfVelocityVirtual, 1.0f);
        //float4 correctedVirtualPos = float4(HandleCollision(updatedVirtualPos.xyz), 1.0f);
        
        //correctedVirtualPos = ApplyStrainLimiting(prevPosForVirtual, correctedVirtualPos);
        
        VirtualHairInfo[velAccessIndex].Position = updatedVirtualPos;
    }
    
    // Clear out forces 
    ClearArray(forces);
    ClearArray(virtualForces);
    
    // Get updated forces, second pass
    CalculateRestitutionForces(hairStartInd, forces, virtualForces, true);

    [unroll(MAX_HAIR_SEGMENTS)]
    for (uint velUpdateIndex = 0; velUpdateIndex < HairSegmentCount + 1; velUpdateIndex++)
    {
        uint velAccessIndexSecPass = velUpdateIndex + hairStartInd;
        if (velUpdateIndex > 0)
        {
            forces[velUpdateIndex] += ResultingForces;
            
            if (DeltaTime >= EPSILON)
            {
                float3 halfVelocity = HairInfo[velAccessIndexSecPass].Velocity.xyz + DeltaTime * 0.5f * forces[velUpdateIndex] / Mass;
                float3 updatedVel = (2.0f * halfVelocity) - HairInfo[velAccessIndexSecPass].Velocity.xyz;
                HairInfo[velAccessIndexSecPass].Velocity = float4(updatedVel, 1.0f);
            }
        }
        
        virtualForces[velUpdateIndex] += ResultingForces;
        if (DeltaTime >= EPSILON)
        {
            float3 halfVelocityVirtual = VirtualHairInfo[velAccessIndexSecPass].Velocity.xyz + DeltaTime * 0.5f * virtualForces[velUpdateIndex] / Mass;
            float3 updatedVirtualVel = (2.0f * halfVelocityVirtual) - VirtualHairInfo[velAccessIndexSecPass].Velocity.xyz;
            VirtualHairInfo[velAccessIndexSecPass].Velocity = float4(updatedVirtualVel, 1.0f);
        }
    }
    
    
}
