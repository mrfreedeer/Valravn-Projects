#include "Constants.hlsl"
#include "MathCommon.hlsl"

struct vs_input
{
    float3 localPosition : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    uint vId : SV_VertexId;
    
};

struct hull_out
{
    float4 position : SV_Position;
    float4 worldPosition : POSITION;
    float3 previousWorldPosition : POSITION1;
    float3 nextWorldPostion : POSITION2;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    uint vId : VERTEXID;
};

struct vs_out
{
    float4 position : SV_Position;
    float4 worldPosition : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    uint vId : VERTEXID;
};

struct hull_tess_out
{
    float EdgesTessellation[2] : SV_TessFactor;
};

struct domain_out
{
    float4 worldPosition : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct gs_out
{
    float4 position : SV_Position;
    float4 worldPosition : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};

struct Cell
{
    int3 Velocity;
    int Density;
};

float3 ComputeSingleScattering(Light light, gs_out worldInfo)
{
    float3 result;
    
    float lightTheta = 0.0f; // theta_i
    float viewTheta = 0.0f; // theta_r
    float lightPhi = 0.0f; // phi_i
    float viewPhi = 0.0f; // phi_r
    
    float avgTheta = 0.0f; // theta_h
    float deltaTheta = 0.0f; // theta_d
    float avgPhi = 0.0f; //  phi_h
    float phi = 0.0f; //  phi
    
    
    static const float scaleShiftRad = ConvertToRadians(ScaleShift);
    
    
    static const float roughness = ConvertToRadians(LongitudinalWidth);
    
    float scaleShiftR = 0.0f;
    float roughnessR = 0.0f;
   
    float roughnessTT = 0.0f;
    float scaleShiftTT = 0.0f;
     
    float roughnessTRT = 0.0f;
    float scaleShiftTRT = 0.0f;
    
    float roughnessSqr = roughness * roughness;
    
    float4 usedColor = worldInfo.color;
    if (UseModelColor)
    {
        usedColor = ModelColor;
    }
    
    if (UseUnrealParameters)
    {
        scaleShiftR = -2.0f * scaleShiftRad;
        roughnessR = roughnessSqr;
    
        scaleShiftTT = scaleShiftRad;
        roughnessTT = roughnessSqr * 0.5f;
    
        scaleShiftTRT = 4.0f * scaleShiftRad;
        roughnessTRT = 2.0f * roughnessSqr;
    }
    else
    {
        scaleShiftR = scaleShiftRad;
        roughnessR = roughness;
    
        scaleShiftTT = -scaleShiftR * 0.5f;
        roughnessTT = roughness * 0.5f;
    
        scaleShiftTRT = -scaleShiftRad * 1.5f;
        roughnessTRT = 2.0f * roughness;
    }
  
    
    
    float3 lightDir = light.Position - worldInfo.worldPosition.xyz;
    lightDir = normalize(lightDir);
    
    float3 viewDir = EyePosition - worldInfo.worldPosition.xyz;
    viewDir = normalize(viewDir);
    
    // Other basis from the orthonormal basis used to study hair's cross section in Marschner's paper
    float3 wBasis = float3(0.0f, 0.0f, 0.0f);
    float3 vBasis = float3(0.0f, 0.0f, 0.0f);
    
    GetOrthonormalBasisFrisvad(worldInfo.tangent, vBasis, wBasis); // I honestly do not see any major difference with H Moeller's ortho normal
    wBasis = normalize(wBasis);
    vBasis = normalize(vBasis);
    
    lightTheta = GetAngleBetDirAndPlane(lightDir, worldInfo.tangent); // TODO make sure the direction of the light vector is right 
    viewTheta = GetAngleBetDirAndPlane(viewDir, worldInfo.tangent); // TODO make sure the direction of the view vector is right
    
    avgTheta = (lightTheta + viewTheta) * 0.5f;
    deltaTheta = (viewTheta - lightTheta) * 0.5f;
    
    float3 projectedLight = ProjectVecOntoPlane(lightDir, worldInfo.tangent);
    float3 projectedView = ProjectVecOntoPlane(viewDir, worldInfo.tangent);
    
    projectedLight = normalize(projectedLight);
    projectedView = normalize(projectedView);
    
    lightPhi = GetAngleBetVectorsAroundPlane(projectedLight, vBasis, worldInfo.tangent);
    viewPhi = GetAngleBetVectorsAroundPlane(projectedView, vBasis, worldInfo.tangent);
   
    
    phi = viewPhi - lightPhi;
    avgPhi = (viewPhi + lightPhi) * 0.5f; // On Epic's approximation, this is probably not used
    // These are values suggested in Marschner's paper
    

    float3 NR = ComputeAzimuthalR(lightDir, viewDir, deltaTheta, phi);
    float3 NTT = ComputeAzimuthalTT(lightDir, viewDir, deltaTheta, phi, usedColor);
    float3 NTRT = ComputeAzimuthalTRT(lightDir, viewDir, deltaTheta, phi, usedColor);
    
    //float3 NR = ComputeAzimuthal(0, lightDir, viewDir, deltaTheta, phi, ModelColor); // 0 bounces
    //float3 NTT = ComputeAzimuthal(1, lightDir, viewDir, deltaTheta, phi, ModelColor); // 1 bounce
    //float3 NTRT = ComputeAzimuthal(2, lightDir, viewDir, deltaTheta, phi, ModelColor); // 2 bounces
    
    float MR = ComputeLongitudinalScattering(scaleShiftR, roughnessR, lightTheta, viewTheta); // 0 bounces
    float MTT = ComputeLongitudinalScattering(scaleShiftTT, roughnessTT, lightTheta, viewTheta); // 1 bounce
    float MTRT = ComputeLongitudinalScattering(scaleShiftTRT, roughnessTRT, lightTheta, viewTheta); // 2 bounces
    
    float cosAvgPhi = abs(cos(avgPhi));
    
    float3 singleScattering = (NR * MR * SpecularMarschner * cosAvgPhi) + (MarschnerTransmCoeff) * (NTT * MTT) + MarschnerTRTCoeff * (NTRT * MTRT); // These coefficients resulted in slightly better results

    singleScattering /= pow(cos(deltaTheta), 2);
    
    return singleScattering;
}

float3 HandleCollision(float3 position)
{
    float3 resultingPosition = position;
    for (int objectIndex = 0; objectIndex < MAX_COLLISION_OBJECTS; objectIndex++)
    {
        CollisionObject collisionObj = CollisionObjects[objectIndex];
        if (collisionObj.Radius > 0.0f)
        {
            resultingPosition = PushPointOutOfSphere(resultingPosition, collisionObj.Position, collisionObj.Radius + CollisionTolerance);
        }
    }
       
    return resultingPosition;

}

float4 ApplyStrainLimiting(float4 prevPosition, float4 currentPosition)
{
  
    float4 newPosition = currentPosition;
    
    float4 dispToCurrent = currentPosition - prevPosition;
    
    float distSqr = dot(dispToCurrent.xyz, dispToCurrent.xyz); // Same as Length Squared
    float maxDist = SegmentLength * 1.1f;
    float maxDistSqr = maxDist * maxDist; // Article suggests a  max of 10% straining
    
    if (distSqr >= maxDistSqr)
    {
        float4 dir = normalize(dispToCurrent);
        newPosition = prevPosition + (dir * maxDist);
    }
    
    return newPosition;
    
}

float4 ApplyStrainLimiting(float4 prevPosition, float4 currentPosition, float3 halfVelocity, float deltaSeconds)
{
  
    float4 newPosition = currentPosition + float4(halfVelocity, 1.0f) * deltaSeconds;
    
    float4 dispToCurrent = newPosition - prevPosition;
    
    float distSqr = dot(dispToCurrent.xyz, dispToCurrent.xyz); // Same as Length Squared
    float maxDist = SegmentLength * 1.1f;
    float maxDistSqr = maxDist * maxDist; // Article suggests a  max of 10% straining
    
    if (distSqr >= maxDistSqr)
    {
        float4 dir = normalize(dispToCurrent);
        newPosition = prevPosition + (dir * maxDist);
    }
    
    return newPosition;
    
}

// Calculate how much the spring force should be according to Mass-Spring A.Selle's paper
float3 GetSpringForce(float3 displacement, float3 velocityDelta, float stiffness, float initialLength)
{
    float3 dir = normalize(displacement);
    
    float initialAndDamping = initialLength + DampingCoefficient;
    
   
    //float stiffOverInitial = (IsHairCurly) ? (stiffness / initialAndDamping) : (stiffness / initialLength);
    float stiffOverInitial = (stiffness / initialAndDamping);
    
    
    float firstTermCoeff = dot(displacement, dir);
    firstTermCoeff -= initialLength;
    firstTermCoeff *= (stiffOverInitial);
    
    float3 firstTerm = dir * firstTermCoeff;
    
    float secondCoeff = (DeltaTime * stiffness / (initialAndDamping));
    
    float3 secondTerm = dot(velocityDelta, dir) * dir;
    secondTerm *= secondCoeff;
    
    float3 resultingForce = firstTerm + secondTerm;
    
    return resultingForce;
}

