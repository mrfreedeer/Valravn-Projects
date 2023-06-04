#define MAX_LIGHTS 8
#define MAX_COLLISION_OBJECTS 30
#define POINT_LIGHT 0
#define SPOT_LIGHT 1

struct Light
{
    bool Enabled;
    float3 Position;
	//------------- 16 bytes
    float3 Direction;
    int LightType; // 0 Point Light, 1 SpotLight
	//------------- 16 bytes
    float4 Color;
	//------------- 16 bytes // These are some decent default values
    float SpotAngle;
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
	//------------- 16 bytes
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

cbuffer LightConstants : register(b1)
{
    float3 DirectionalLight;
    float PaddingDirectionalLight;
    float4 DirectionalLightIntensity;
    float4 AmbientIntensity;
    Light Lights[MAX_LIGHTS];
    float MaxOcclusionPerSample;
    float SSAOFalloff;
    float SampleRadius;
    int SampleSize;
}

cbuffer CameraConstants : register(b2)
{
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
};

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelMatrix;
    float4 ModelColor;
    float4 ModelPadding;
}


cbuffer HairConstants : register(b4)
{
    float3 EyePosition;
    float HairWidth;
    // -------------------------------------------
    float DiffuseCoefficient;
    float SpecularCoefficient;
    uint SpecularExponent;
    float StartTime;
    // -------------------------------------------
    uint InterpolationFactor;
    uint InterpolationFactorMultiStrand;
    float InterpolationRadius;
    float TessellationFactor;
    // -------------------------------------------
    float LongitudinalWidth;
    float ScaleShift;
    bool UseUnrealParameters;
    float SpecularMarschner;
    // -------------------------------------------
    float DeltaTime;
    float3 ExternalForces;
    // -------------------------------------------
    float3 Displacement;
    float Gravity;
    // -------------------------------------------
    float SegmentLength;
    float BendInitialLength;
    float TorsionInitialLength;
    float DampingCoefficient;
    // -------------------------------------------
    float EdgeStiffness;
    float BendStiffness;
    float TorsionStiffness;
    uint IsHairCurly;
    // -------------------------------------------
    float Mass;
    uint SimulationAlgorithm;
    float GridCellWidth;
    float GridCellHeight;
    // -------------------------------------------
    float FrictionCoefficient;
    int3 GridDimensions;
    // -------------------------------------------
    float CollisionTolerance;
    uint HairSegmentCount;
    float StrainLimitingCoefficient;
    uint InterpolateUsingRadius;
    // -------------------------------------------
    //float4 LimitingPlane;
    // -------------------------------------------
    float MarschnerTransmCoeff;
    float MarschnerTRTCoeff;
    uint UseModelColor;
    uint InvertLightDir;

}


struct CollisionObject
{
    float3 Position;
    float Radius;
};

cbuffer CollisionConstants : register(b5)
{
    CollisionObject CollisionObjects[MAX_COLLISION_OBJECTS];
}

struct HairData
{
    float4 Position;
    float4 Velocity;
};