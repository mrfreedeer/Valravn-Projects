#define MAX_LIGHTS 8
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
    float4x4 InvertedProjectionMatrix;
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

Texture2D<float> depthTexture : register(t0);
SamplerState diffuseSampler : register(s0);

// input to the vertex shader - for now, a special input that is the index of the vertex we're drawing
struct vs_input_t
{
    uint vidx : SV_VERTEXID; // SV_* stands for System Variable (ie, built-in by D3D11 and has special meaning)
                           // in this case, is the the index of the vertex coming in.
};

// Output from Vertex Shader, and Input to Fragment Shader
struct ps_input_t
{
    float4 position : SV_POSITION;
    float2 uv : TEX_COORD;
};

static float3 FS_POS[3] =
{
    float3(-1.0f, -1.0f, 0.0f),
   float3(-1.0f, 3.0f, 0.0f),
   float3(3.0f, -1.0f, 0.0f),
};

static float2 FS_UVS[3] =
{
    float2(0.0f, 1.0f),
   float2(0.0f, -1.0f),
   float2(2.0f, 1.0f),
};

float3 GetEyePositionFromUV(float2 uv, float depth)
{
    float2 ndcPos = (uv * 2.0f) - 1.0f;
    float4 pos = float4(ndcPos.x, -ndcPos.y, depth, 1.0f);
    
    float4 eyeSpacePos = mul(InvertedProjectionMatrix, pos);
    eyeSpacePos /= eyeSpacePos.w;
    
    return eyeSpacePos.xyz;
}

ps_input_t VertexMain(vs_input_t input)
{
    ps_input_t output = (ps_input_t) 0;
    
    output.position = float4(FS_POS[input.vidx], 1.0f);
    output.uv = FS_UVS[input.vidx];
    
    return output;
}

float Attenuate(Light light, float distance)
{
    float attenuation = (light.ConstantAttenuation + light.LinearAttenuation * distance + light.QuadraticAttenuation * distance * distance);
    if (attenuation == 0)
    {
        return 1.0f;
    }
    return 1.0f / attenuation;
}

float4 CalculateDiffusePointLight(Light light, float4 Position, float3 Normal)
{
    float3 dispToLight = light.Position - Position.xyz;
    float3 L = normalize(dispToLight);
    float attenuation = Attenuate(light, length(dispToLight));
    
    return saturate(dot(Normal, L)) * light.Color * attenuation;
}

float4 PixelMain(ps_input_t input) : SV_Target0
{
    uint2 depthDims;
    depthTexture.GetDimensions(depthDims.x, depthDims.y);
    const float2 epsilon = (1.0f.xx / float2(depthDims));
    static float maxDepth = 1.0f;
    static float4x4 InvViewMat = transpose(ViewMatrix);
    
    float2 deltaX = float2(epsilon.x, 0.0f);
    float2 deltaY = float2(0.0f, epsilon.y);
    
    float2 topUV = input.uv - deltaY;
    float2 bottomUV = input.uv + deltaY;
    
    float2 rightUV = input.uv + deltaX;
    float2 leftUV = input.uv - deltaX;
    
    
    float depth = depthTexture.Sample(diffuseSampler, input.uv);
    if (depth >= maxDepth)
    {
        discard;
    }
    
    float topDepth = depthTexture.Sample(diffuseSampler, topUV);
    float bottomDepth = depthTexture.Sample(diffuseSampler, bottomUV);
    float leftDepth = depthTexture.Sample(diffuseSampler, leftUV);
    float rightDepth = depthTexture.Sample(diffuseSampler, rightUV);
    
    float3 pixelEyePos = GetEyePositionFromUV(input.uv, depth);
    float3 rightNeighborEyePos = GetEyePositionFromUV(rightUV, rightDepth) - pixelEyePos;
    float3 leftNeighborEyepos = pixelEyePos - GetEyePositionFromUV(leftUV, leftDepth);
    
    float3 topNeighborEyePos = GetEyePositionFromUV(topUV, topDepth) - pixelEyePos;
    float3 bottomNeighborEyepos = pixelEyePos - GetEyePositionFromUV(bottomUV, bottomDepth);
    
    float3 difX = leftNeighborEyepos;
    float3 difY = topNeighborEyePos;
    
    if (abs(rightNeighborEyePos.z) < abs(leftNeighborEyepos.z))
    {
        difX = rightNeighborEyePos;
    }
    
    if (abs(bottomNeighborEyepos.z) < abs(topNeighborEyePos.z))
    {
        difY = bottomNeighborEyepos;
    }
    
    float3 normal = normalize(cross(difX, difY));
   
    normal = normalize(mul(InvViewMat, float4(normal, 0.0f)).xyz);
    float4 worldPos = mul(InvViewMat, float4(pixelEyePos, 1.0f));
    
    float4 diffuseLight = CalculateDiffusePointLight(Lights[0], worldPos, normal) * float4(0.0f, 0.0f, 1.0f, 1.0f);
    
    //return float4(normal, 1.0f);
    return diffuseLight;
}
