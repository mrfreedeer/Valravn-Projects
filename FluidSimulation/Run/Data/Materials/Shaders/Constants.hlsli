#define MAX_LIGHTS 8
#define PI 3.14159265f

struct ms_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
};

struct ps_input_t
{
    float4 position : SV_Position;
    float3 eyeSpacePosition : POSITION0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct ps_output_t
{
    float worldDepth : SV_Depth;
};

struct Meshlet
{
    uint VertexCount;
    uint VertexOffset;
    uint PrimCount;
    uint PrimOffset;
    float4 Color;
};

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

cbuffer CameraConstants : register(b0)
{
    float4x4 ProjectionMatrix;
    float4x4 ViewMatrix;
};

cbuffer ModelConstants : register(b1)
{
    float4x4 ModelMatrix;
    float4 ModelColor;
    float4 ModelPadding;
}

cbuffer LightConstants : register(b2)
{
    float3 DirectionalLight;
    float PaddingDirectionalLight;
    float4 DirectionalLightIntensity;
    float4 AmbientIntensity;
    Light Lights[MAX_LIGHTS];
}


cbuffer GameConstants : register(b3)
{
    float3 EyePosition;
    float SpriteRadius;
    float3 CameraUp;
    float KernelRadius;
    float3 CameraLeft;
    uint ParticleCount;
    float3 Forces; // Gravity included
    float DeltaTime; // In Seconds
    float3 BoundsMins;
    float RestDensity;
    float3 BoundsMaxs;
    uint Padding;
    float4 ParticleColor;
}

cbuffer ComputeConstants : register(b0, space1)
{
    row_major uint4x4 SortUConstants;
}


struct Particle
{
    float3 Position;
    float Density;
    float3 PredictedPosition;
    float Lambda;
    float3 PrevPosition;
    uint Padding;
    float3 Velocity;
    uint Padding2;
    float3 Gradient;
    uint Padding3;
};

struct HashInfo
{
    uint Hash;
    uint ModdedHash;
    uint AccessIndex;
};


static const int3 NeighborOffsets[] = // 27 total
{
    { -1, -1, -1 },
    { -1, -1, 0 },
    { -1, -1, 1 },
    { -1, 0, -1 },
    { -1, 0, 0 },
    { -1, 0, 1 },
    { -1, 1, -1 },
    { -1, 1, 0 },
    { -1, 1, 1 },
    { 0, -1, -1 },
    { 0, -1, 0 },
    { 0, -1, 1 },
    { 0, 0, -1 },
    { 0, 0, 0 },
    { 0, 0, 1 },
    { 0, 1, -1 },
    { 0, 1, 0 },
    { 0, 1, 1 },
    { 1, -1, -1 },
    { 1, -1, 0 },
    { 1, -1, 1 },
    { 1, 0, -1 },
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 1, 1, -1 },
    { 1, 1, 0 },
    { 1, 1, 1 },
};
