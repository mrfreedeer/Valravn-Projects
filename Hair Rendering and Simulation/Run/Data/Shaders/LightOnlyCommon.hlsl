#include "Constants.hlsl"
#include "MathCommon.hlsl"

struct vs_input_t
{
    float3 localPosition : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2g_t
{
    float4 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float4 worldPosition : WORLDPOSITION;
};

struct g2p_t
{
    float4 position : SV_Position;
    float3 worldPosition : WORLD_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
};


float3 ComputeSingleScattering(Light light, g2p_t worldInfo)
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
    
    GetOrthonormalBasisFrisvad(worldInfo.tangent, wBasis, vBasis);
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
    

    float3 NR = ComputeAzimuthal(0, lightDir, viewDir, deltaTheta, phi, ModelColor); // 0 bounces
    float3 NTT = ComputeAzimuthal(1, lightDir, viewDir, deltaTheta, phi, ModelColor); // 1 bounce
    float3 NTRT = ComputeAzimuthal(2, lightDir, viewDir, deltaTheta, phi, ModelColor); // 2 bounces
    
    float MR = ComputeLongitudinalScattering(scaleShiftR, roughnessR, lightTheta, viewTheta); // 0 bounces
    float MTT = ComputeLongitudinalScattering(scaleShiftTT, roughnessTT, lightTheta, viewTheta); // 1 bounce
    float MTRT = ComputeLongitudinalScattering(scaleShiftTRT, roughnessTRT, lightTheta, viewTheta); // 2 bounces
    
    float cosAvgPhi = abs(cos(avgPhi));
    float3 singleScattering = (NR * MR * cosAvgPhi * SpecularMarschner) + 0.05 * (NTT * MTT) + 0.25f * (NTRT * MTRT); // These coefficients resulted in slightly better results
    float cosDeltaTheta = cos(deltaTheta);
    
    
    singleScattering *= (1.0f / (cosDeltaTheta * cosDeltaTheta));
    
    
    return singleScattering;
}