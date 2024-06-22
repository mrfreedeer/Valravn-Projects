#define MAX_LIGHTS 8
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
    float4 CameraUp;
    float4 CameraLeft;
}

float4 PixelMain(ps_input_t input): SV_Target0
{
    float2 posInCircle = (input.uv * 2.0f) - 1.0f;
    float radiusSqr = dot(posInCircle, posInCircle);
    
    if (radiusSqr > 1.0f)
        discard;
    float thickness = sqrt(1.0f - radiusSqr) * 0.55f;
    return float4(thickness, 0.f, 0.f, thickness);
}
