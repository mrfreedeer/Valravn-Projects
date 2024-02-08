#define MAX_LIGHTS 8
#define POINT_LIGHT 0
#define SPOT_LIGHT 1

struct vs_input_t
{
    float3 localPosition : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    
};

struct v2p_t
{
    float4 position : SV_Position;
    float4 worldPosition : POSITION;
    float4 lightDepthPosition : POSITION1;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
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

cbuffer LightConstants : register(b1)
{
    float3 DirectionalLight;
    float PaddingDirectionalLight;
    float4 DirectionalLightIntensity;
    float4 AmbientIntensity;
    Light Lights[MAX_LIGHTS];
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

cbuffer ShadowConstants : register(b4)
{
    float DepthBias;
    uint UseBoxSampling;
    uint UsePCF;
    uint PCFSampleCount;
}

Texture2D diffuseTexture : register(t0);
Texture2D depthTexture : register(t1);
StructuredBuffer<float2> PoissonSamplePoints : register(t3);
SamplerState diffuseSampler : register(s0);
SamplerComparisonState shadowSampler : register(s1);

void GetOrthonormalBasisFrisvad(float3 iBasis, out float3 jBasis, out float3 kBasis) // There are multiple ways of finding an orthonormal basis. This has been studied to be fast as it does not need square roots. Frisvad method
{
    if (iBasis.z < -0.9999999f) // Handle the singularity
    {
        jBasis = float3(0.0f, -1.0f, 0.0f);
        kBasis = float3(-1.0f, 0.0f, 0.0f);
        return;
    }
    const float a = 1.0f / (1.0f + iBasis.z);
    const float b = -iBasis.x * iBasis.y * a;
    jBasis = float3(1.0f - iBasis.x * iBasis.x * a, b, -iBasis.x);
    kBasis = float3(b, 1.0f - iBasis.y * iBasis.y * a, -iBasis.y);
}

float1 GetFractionWithin(float1 inValue, float1 inStart, float1 inEnd)
{
    if (inStart == inEnd)
        return 0.5f;
    float1 range = inEnd - inStart;
    return (inValue - inStart) / range;
}

float1 Interpolate(float1 outStart, float1 outEnd, float1 fraction)
{
    return outStart + fraction * (outEnd - outStart);
}

float1 RangeMap(float1 inValue, float1 inStart, float1 inEnd, float1 outStart, float1 outEnd)
{
    float1 fraction = GetFractionWithin(inValue, inStart, inEnd);
    return Interpolate(outStart, outEnd, fraction);
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

float CalculateSpotIntensity(Light light, float3 L)
{
    float angleInRadians = radians(light.SpotAngle);
    float minIntensity = cos(angleInRadians);
    float maxIntensity = (minIntensity + 1.0f) * 0.5f;
    
    float cosAngle = dot(light.Direction, -L);
    
    return smoothstep(minIntensity, maxIntensity, cosAngle);
}

float4 CalculateDiffuseSpotLight(Light light, float4 Position, float3 Normal)
{
    float3 dispToLight = light.Position - Position.xyz;
    float3 L = normalize(dispToLight);
    float attenuation = Attenuate(light, length(dispToLight));
    
    float spotIntensity = CalculateSpotIntensity(light, L);
    
    return saturate(dot(Normal, L)) * light.Color * attenuation * spotIntensity;
}

float4 CalculateDiffusePointLight(Light light, float4 Position, float3 Normal)
{
    float3 dispToLight = light.Position - Position.xyz;
    float3 L = normalize(dispToLight);
    float attenuation = Attenuate(light, length(dispToLight));
    
    return saturate(dot(Normal, L)) * light.Color * attenuation;
}

float4 ComputeDiffuseLighting(float4 Point, float3 Normal)
{
    float4 totalDiffuse = 0.0f;
    [unroll]
    for (int lightIndex = 0; lightIndex < MAX_LIGHTS; lightIndex++)
    {
        if (!Lights[lightIndex].Enabled)
            continue;
        switch (Lights[lightIndex].LightType)
        {
            case POINT_LIGHT:
                totalDiffuse += CalculateDiffusePointLight(Lights[lightIndex], Point, Normal);
                break;
            case SPOT_LIGHT:
                totalDiffuse += CalculateDiffuseSpotLight(Lights[lightIndex], Point, Normal);
                break;
        }
        
    }
    totalDiffuse = saturate(totalDiffuse);
    return totalDiffuse;
}

float4x4 CreatePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{
    float4x4 perspectiveProj = float4x4(0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx);
    float angle = fovYDegrees * 0.5f;
    float angleRadians = radians(angle);
    
    float sy = cos(angleRadians) / sin(angleRadians);
    float sx = sy / aspect;
    float sz = zFar / (zFar - zNear);
    float tz = (zNear * zFar) / (zNear - zFar);

    perspectiveProj._11 = sx;
    perspectiveProj._22 = sy;
    perspectiveProj._33 = sz;
    perspectiveProj._43 = tz;

    return perspectiveProj;
}
v2p_t VertexMain(vs_input_t input)
{
    v2p_t v2p;
    float4 position = float4(input.localPosition, 1);
    float4 normal = float4(input.normal, 0);
    
    
    float4 normalWorldSpace = mul(ModelMatrix, normal);
    float4 worldPosition = mul(ModelMatrix, position);
    float4 worldToViewPos = mul(ViewMatrix, worldPosition);
    

    float4 lightDepthPos = mul(Lights[0].ViewMatrix, worldPosition);
    lightDepthPos = mul(Lights[0].ProjectionMatrix, lightDepthPos);
    
    v2p.position = mul(ProjectionMatrix, worldToViewPos);
    v2p.worldPosition = worldPosition;
    v2p.lightDepthPosition = lightDepthPos;

    v2p.color = input.color;
    v2p.uv = input.uv;
    v2p.normal = normalWorldSpace.xyz;
    
    return v2p;
}

float CustomNoise(float2 seed)
{
    float dot_product = dot(seed, float2(12.9898, 94.673));
    return frac(sin(dot_product) * 43758.5453);
}


float4 CalculateShadowFactor(float3 position)
{
    uint width = 0;
    uint height = 0;
    depthTexture.GetDimensions(width, height);
    
    float dx = 1.0f / float(width);
    float dy = 1.0f / float(height);
    
    
    const float2 offsets[9] =
    {
        float2(-dx, -dy), float2(0.0f, -dy), float2(dx, -dy),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dy), float2(0.0f, +dy), float2(dx, +dy)
    };
    
    float4 percentLitBox = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    [unroll]
    for (uint sampleIndex = 0; sampleIndex < 9; sampleIndex++)
    {
        percentLitBox += depthTexture.SampleCmpLevelZero(shadowSampler, position.xy + offsets[sampleIndex], position.z - DepthBias);
        
    }
    percentLitBox /= 9.0f;
    
    
    float4 percentLitPCF = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
     [unroll(64)]
    for (uint PCFSampleIndex = 0; PCFSampleIndex < PCFSampleCount; PCFSampleIndex++)
    {        
        float rotation = CustomNoise(position.xy) * 360.0f;
        float rotationRadians = radians(rotation);
        float cosRot = cos(rotationRadians);
        float sinRot = sin(rotationRadians);
        
        float2 rotIBasis = float2(cosRot, sinRot);
        float2 rotJBasis = float2(-sinRot, cosRot);
        
        float2 poissonOffset = PoissonSamplePoints[PCFSampleIndex].x * rotIBasis + PoissonSamplePoints[PCFSampleIndex].y * rotJBasis;
        poissonOffset.x *= dx;
        poissonOffset.y *= dy;
        
        percentLitPCF += depthTexture.SampleCmpLevelZero(shadowSampler, position.xy + poissonOffset, position.z - DepthBias);
    }
    
    float PCFSamplesFloat = float(PCFSampleCount);
    percentLitPCF /= PCFSamplesFloat;
    
    
    return (UsePCF) ? percentLitPCF : percentLitBox;
}


float4 PixelMain(v2p_t input) : SV_Target0
{
    // W division should occur here, as the interpolated light view position is accurate, in VShader it's not
    input.lightDepthPosition.xyz /= input.lightDepthPosition.w;
    input.lightDepthPosition.x = (input.lightDepthPosition.x * 0.5f) + 0.5f;
    input.lightDepthPosition.y = 0.5f - (input.lightDepthPosition.y * 0.5f);
    
    float4 ambient = AmbientIntensity;
    float3 normalizedNormal = normalize(input.normal);
    float4 directional = DirectionalLightIntensity * saturate(dot(normalizedNormal, -DirectionalLight));
    float4 lightColor = ambient + directional + ComputeDiffuseLighting(input.worldPosition, normalizedNormal);
    float4 diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    float4 resultingColor = lightColor * diffuseColor * input.color;
    //float4 resultingColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * ModelColor;
    

    if (input.lightDepthPosition.z >= 0.0f && input.lightDepthPosition.z < 1.0f)
    {
        if (input.lightDepthPosition.x >= 0.0f && input.lightDepthPosition.x <= 1.0f)
        {
            if (input.lightDepthPosition.y >= 0.0f && input.lightDepthPosition.y <= 1.0f)
            {
                float4 percentLit = float4(0.0f, 0.0f, 0.0f, 0.0f);
                if (UseBoxSampling || UsePCF)
                {
                    percentLit = CalculateShadowFactor(input.lightDepthPosition.xyz);
                }
                else
                {
                    percentLit = depthTexture.SampleCmpLevelZero(shadowSampler, input.lightDepthPosition.xy, input.lightDepthPosition.z - DepthBias);
                }
        //percentLit += float4(AmbientIntensity.xyzw);
                saturate(percentLit);
                return resultingColor * percentLit;
            }
        
        }
    }
   
    
    //clip(resultingColor.a - 0.5f);
    return input.color * diffuseColor * AmbientIntensity;
}
