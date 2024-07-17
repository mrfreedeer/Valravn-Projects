#define EPSILON 0.00001f
#include "Constants.hlsli"


RWStructuredBuffer<Particle> Particles : register(u0);
RWStructuredBuffer<HashInfo> HashArray : register(u1);
RWStructuredBuffer<uint> Offsets : register(u2);


float3 KeepParticleInBounds(in float3 position)
{
	
	//Vec3 correctedPosition = bounds.GetNearestPoint(position);

    if (position.z < BoundsMins.z)
        position.z = BoundsMins.z;
	//if (position.z > bounds.m_maxs.z) {
	//	correctedPosition.z = bounds.m_maxs.z;
	//}

    if (position.x < BoundsMins.x)
        position.x = BoundsMins.x;
    
    if (position.x > BoundsMaxs.x)
        position.x = BoundsMaxs.x;

    if (position.y < BoundsMins.y)
        position.y = BoundsMins.y;
    
    if (position.y > BoundsMaxs.y)
        position.y = BoundsMaxs.y;


    return position;
}


[numthreads(1024, 1, 1)]
void ApplyForces(uint3 threadId : SV_DispatchThreadID)
{
    const float3 forceUpdate = DeltaTime * Forces;
    Particle accessedParticle = Particles[threadId.x];
    accessedParticle.Velocity += forceUpdate;
    accessedParticle.PredictedPosition = accessedParticle.Position + DeltaTime * accessedParticle.Velocity;
    
    float3 correctedPos = KeepParticleInBounds(accessedParticle.PredictedPosition);
    
    if (DeltaTime == 0.0f)
    {
        accessedParticle.Velocity = float3(0.0f, 0.0f, 0.0f);
    }
    else
    {
        accessedParticle.Velocity += (correctedPos - accessedParticle.PredictedPosition) / DeltaTime;
    }
    
    accessedParticle.PredictedPosition = correctedPos;
    
    Particles[threadId.x] = accessedParticle;
}

int3 GetCoordsForPosition(float3 position)
{
    static const float recipKernelRadius = 1.0f / KernelRadius;
    position *= recipKernelRadius;
    int3 coords = int3(floor(position));
    return coords;
}

int HashCoords(int3 coords)
{
    static const int p1 = 73856093 * coords.x;
    static const int p2 = 19349663 * coords.y;
    static const int p3 = 83492791 * coords.z;

    return p1 + p2 + p3;
}

float Poly6Kernel(float distance)
{
    static const float kernelRadiusSqr = KernelRadius * KernelRadius;
    static const float kernelRadiusPwr9 = (kernelRadiusSqr * kernelRadiusSqr) * (kernelRadiusSqr * kernelRadiusSqr) * KernelRadius;
    static const float kernelCoeff = 315.0f / (64.0f * (PI) * kernelRadiusPwr9);
    
    bool skipCal = (distance > KernelRadius) || (distance < 0.0f);
    if (skipCal)
        return 0.0f;
    
    const float dSqr = distance * distance;

    float distanceCoeff = (kernelRadiusSqr - dSqr);
    distanceCoeff *= distanceCoeff * distanceCoeff;

    float kernelValue = kernelCoeff * distanceCoeff;
    return kernelValue;
}

float3 SpikyKernelGradient(in float3 displacement, float distance)
{
    static const float kernelRadiusPwr6 = (KernelRadius * KernelRadius) * (KernelRadius * KernelRadius) * (KernelRadius * KernelRadius);

    static const float kernelCoefficient = -45.0f / (PI * kernelRadiusPwr6);

    if (distance > KernelRadius)
        return float3(0.0f, 0.0f, 0.0f);

    float distanceSqr = KernelRadius - distance;
    distanceSqr *= distanceSqr;
    const float kernelValue = (kernelCoefficient * distanceSqr);

    float3 gradientValue = normalize(displacement) * (kernelValue);

    return gradientValue;
}

float3 UpdatePositionDelta(Particle particle)
{
    if (particle.Density < RestDensity)
        return 0.0f.xxx;
	
    float3 deltaPos = 0.0f.xxx;
    const int3 cell = GetCoordsForPosition(particle.Position);

    for (int neighborInd = 0; neighborInd < 27; neighborInd++)
    {
        const int3 offset = NeighborOffsets[neighborInd];
        const int3 neighborCell = cell + offset;
        
        const int neighborHash = HashCoords(neighborCell);
        const int moddedHash = neighborHash % ParticleCount;
        const uint hashStartInd = Offsets[moddedHash];
        
        uint hashAccessInd = hashStartInd;
        HashInfo hashInfo = HashArray[hashAccessInd];
        
        bool foundStart = false;
        while (hashInfo.ModdedHash == moddedHash)
        {
            if (hashInfo.Hash == neighborHash)
            {
                foundStart = true;
                
                Particle otherParticle = Particles[hashInfo.AccessIndex];
             
                float3 displacement = particle.PredictedPosition - otherParticle.PredictedPosition;
                float d = length(displacement);
                float coefficient = otherParticle.Lambda + particle.Lambda;
                float3 gradient = SpikyKernelGradient(displacement, d);
                deltaPos += coefficient * gradient;
            }
            else
            {
                if (foundStart)
                {
                    break;
                }

            }
            hashAccessInd++;
            hashInfo = HashArray[hashAccessInd];
        }

    }
    return deltaPos / RestDensity;
}


[numthreads(1024, 1, 1)]
void CalculateLambda(uint3 threadId : SV_DispatchThreadID)
{
    static float RecipRestDensity = 1.0f / RestDensity;
    
    Particle currentParticle = Particles[threadId.x];
    
    float density = 0.0f;
    float3 gradientSum = float3(0.0f, 0.0f, 0.0f);
    float gradientLengthSqrSum = 0.0f;
    
    const int3 cell = GetCoordsForPosition(currentParticle.Position);
    for (int neighborInd = 0; neighborInd < 27; neighborInd++)
    {
        const int3 offset = NeighborOffsets[neighborInd];
        const int3 neighborCell = cell + offset;
        
        const int neighborHash = HashCoords(neighborCell);
        const int moddedHash = neighborHash % ParticleCount;
        const uint hashStartInd = Offsets[moddedHash];
        
        uint hashAccessInd = hashStartInd;
        HashInfo hashInfo = HashArray[hashAccessInd];
        
        bool foundStart = false;
        while (hashInfo.ModdedHash == moddedHash)
        {
            if (hashInfo.Hash == neighborHash)
            {
                foundStart = true;
                
                Particle otherParticle = Particles[hashInfo.AccessIndex];
                float3 displacement = (currentParticle.PredictedPosition - otherParticle.PredictedPosition);
                float d = length(displacement);
                density += Poly6Kernel(d);
                float3 gradient = SpikyKernelGradient(displacement, d) * RecipRestDensity;
                gradientSum += gradient;
                gradientLengthSqrSum += dot(gradient, gradient);
            }
            else
            {
                if (foundStart)
                {
                    break;
                }

            }
            
            hashAccessInd++;
            hashInfo = HashArray[hashAccessInd];

        }

    }
    
    float densityConstraint = max((density * RecipRestDensity) - 1.0f, 0.0f);
    
    gradientLengthSqrSum += dot(gradientSum, gradientSum);
    
    currentParticle.Lambda = -densityConstraint / (gradientLengthSqrSum + EPSILON);
    currentParticle.Gradient = gradientSum;
    Particles[threadId.x] = currentParticle;
    
    AllMemoryBarrierWithGroupSync();
    
    // Update Position now
    float3 deltaPos = UpdatePositionDelta(currentParticle);
    currentParticle.PredictedPosition += deltaPos;
    Particles[threadId.x] = currentParticle;
}


bool ShouldSwap(uint firstIndex, uint secIndex)
{
    static const uint direction = SortUConstants._11;
    static const uint stageInd = SortUConstants._12;
    static const uint stepInd = SortUConstants._13;
    
    uint firstHash = HashArray[firstIndex].ModdedHash;
    uint secHash = HashArray[secIndex].ModdedHash;
    
    bool shouldSwap = ((firstIndex & stageInd) == direction) && (firstHash > secHash);
    shouldSwap = shouldSwap || (((firstIndex & stageInd) != direction) && (firstHash < secHash));

    return shouldSwap;
}

[numthreads(1024, 1, 1)]
void BitonicSort(uint3 threadId : SV_DispatchThreadID)
{
    static const uint stepInd = SortUConstants._13;
    
    uint secondIndex = (threadId.x ^ stepInd);
    
    if (secondIndex > threadId.x)
    {
        bool swap = ShouldSwap(threadId.x, secondIndex);
    
        if (swap)
        {
            HashInfo temp = HashArray[threadId.x];
            HashArray[threadId.x] = HashArray[secondIndex];
            HashArray[secondIndex] = temp;
        }
    }
}

// FluidSolver::UpdateNeighbors is somewhat split into this and the update function
[numthreads(1024, 1, 1)]
void HashParticles(uint3 threadId : SV_DispatchThreadID)
{
    if (threadId.x >= ParticleCount)
        return;
    
    int3 coords = GetCoordsForPosition(Particles[threadId.x].Position);
    int hash = HashCoords(coords);
    int moddedHash = hash % (ParticleCount);
    
    HashInfo newHashInfo = (HashInfo) (0);
    newHashInfo.Hash = hash;
    newHashInfo.ModdedHash = moddedHash;
    newHashInfo.AccessIndex = threadId.x;
    
    HashArray[threadId.x] = newHashInfo;
}

[numthreads(1024, 1, 1)]
void GenerateOffsets(uint3 threadId : SV_DispatchThreadID)
{
    Offsets[threadId.x] = -1;
    
    AllMemoryBarrierWithGroupSync();
    
    if (threadId.x >= ParticleCount)
        return;
    
    uint prevIndex = (threadId.x == 0) ? ParticleCount - 1 : threadId.x - 1;
    uint prevModHash = HashArray[prevIndex].ModdedHash;
    uint currentModHash = HashArray[threadId.x].ModdedHash;
    
    if (currentModHash != prevModHash)
    {
        Offsets[currentModHash] = threadId.x;
    }
    
}

[numthreads(1024, 1, 1)]
void UpdateParticlesMovement(uint3 threadId : SV_DispatchThreadID)
{
    Particle currentParticle = Particles[threadId.x];
    currentParticle.Velocity = (currentParticle.PredictedPosition - currentParticle.Position) / DeltaTime;
    Particles[threadId.x] = currentParticle;
    static const float reciprRestDensity = 1.0f / RestDensity;
    
    AllMemoryBarrierWithGroupSync();
    
    // Update Viscosity
    const int3 cell = GetCoordsForPosition(currentParticle.Position);
    float3 viscosity = float3(0.0f, 0.0f, 0.0f);
    for (int neighborInd = 0; neighborInd < 27; neighborInd++)
    {
        const int3 offset = NeighborOffsets[neighborInd];
        const int3 neighborCell = cell + offset;
        
        const int neighborHash = HashCoords(neighborCell);
        const int moddedHash = neighborHash % ParticleCount;
        const uint hashStartInd = Offsets[moddedHash];
        
        uint hashAccessInd = hashStartInd;
        HashInfo hashInfo = HashArray[hashAccessInd];
        
        bool foundStart = false;
        while (hashInfo.ModdedHash == moddedHash)
        {
            if (hashInfo.Hash == neighborHash)
            {
                foundStart = true;
                
                Particle otherParticle = Particles[hashInfo.AccessIndex];
                float3 displacement = (currentParticle.PredictedPosition - otherParticle.PredictedPosition);
                float d = length(displacement);
                float3 diffVelocities = (currentParticle.Velocity - otherParticle.Velocity);
                viscosity += diffVelocities * Poly6Kernel(d);
            }
            else
            {
                if (foundStart)
                {
                    break;
                }

            }
            hashAccessInd++;
            hashInfo = HashArray[hashAccessInd];
        }

    }
    
    
    viscosity *= 0.01f;
    currentParticle.Velocity += (viscosity * reciprRestDensity);

    // Update Position
    AllMemoryBarrierWithGroupSync();
    float3 prevPos = currentParticle.PredictedPosition;
    currentParticle.PrevPosition = currentParticle.Position;
    currentParticle.PredictedPosition = KeepParticleInBounds(currentParticle.PredictedPosition);
    currentParticle.Velocity += (currentParticle.PredictedPosition - prevPos) / DeltaTime;
    currentParticle.Position = currentParticle.PredictedPosition;
    
    Particles[threadId.x] = currentParticle;
    
}
