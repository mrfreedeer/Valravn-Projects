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


Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

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

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 ambient = AmbientIntensity;
    float3 normalizedNormal = normalize(input.normal);
    float4 directional = DirectionalLightIntensity * saturate(dot(normalizedNormal, -DirectionalLight));
    float4 lightColor = ambient + directional + ComputeDiffuseLighting(input.worldPosition, normalizedNormal);
    float4 diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    float4 resultingColor = lightColor * diffuseColor * input.color;
    
    //clip(resultingColor.a - 0.5f);
    return resultingColor;
}


v2p_t VertexMain(vs_input_t input)
{
    v2p_t v2p;
    float4 position = float4(input.localPosition, 1);
    float4 normal = float4(input.normal, 0);
    
    float4 modelTransform = mul(ModelMatrix, position);
    float4 normalWorldSpace = mul(ModelMatrix, normal);
    float4 modelToViewPos = mul(ViewMatrix, modelTransform);
    
    v2p.position = mul(ProjectionMatrix, modelToViewPos);
    v2p.worldPosition = modelTransform;
    v2p.color = input.color;
    v2p.uv = input.uv;
    v2p.normal = normalWorldSpace.xyz;
    
    return v2p;
}