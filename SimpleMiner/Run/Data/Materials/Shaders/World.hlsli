struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_Position;
    float4 worldPosition : WORLDPOSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
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

cbuffer GameConstants : register(b2)
{
    float4 CameraWorldPosition;
    float4 GlobalIndoorLight;
    float4 GlobalOutdoorLight;
    float4 SkyColor;
    float FogStartDistance;
    float FogEndDistance;
    float FogEndAlpha;
    float Time;
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


float4 DiminishingAdd(float4 a, float4 b)
{
    return 1.0f - ((1.0f - a) * (1.0f - b));
}

float4 PixelMain(v2p_t input) : SV_Target0
{
    
    float4 outdoorLightColor = input.color.r * GlobalOutdoorLight;
    float4 indoorLightColor = input.color.g * GlobalIndoorLight;
    
    float4 diffuseLightColor = DiminishingAdd(outdoorLightColor, indoorLightColor);
    float4 diffuseTextureColor = diffuseTexture.Sample(diffuseSampler, input.uv) * ModelColor;
    if (diffuseTextureColor.w < 0.01f)
        discard;
    
    float4 blendedLightColor = diffuseLightColor * diffuseTextureColor;
    
    float3 dispPixelToCamera = input.worldPosition.xyz - CameraWorldPosition.xyz;
    float distToCamera = length(dispPixelToCamera);
    float tForFog = FogEndAlpha * saturate((distToCamera - FogStartDistance) / (FogEndDistance - FogStartDistance));
    
    float finalAlpha = saturate(tForFog + (diffuseTextureColor.a)) * input.color.a;
    
    float3 resultingColor = lerp(blendedLightColor.rgb, SkyColor.rgb, tForFog);
    
    
    
    return float4(resultingColor, finalAlpha);
}

float customHash(float n)
{
    return frac(sin(n) * 43758.5453);
}

float customNoise(float3 x)
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = frac(x);

    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0 + 113.0 * p.z;

    return lerp(lerp(lerp(customHash(n + 0.0), customHash(n + 1.0), f.x),
                   lerp(customHash(n + 57.0), customHash(n + 58.0), f.x), f.y),
               lerp(lerp(customHash(n + 113.0), customHash(n + 114.0), f.x),
                   lerp(customHash(n + 170.0), customHash(n + 171.0), f.x), f.y), f.z);
}


v2p_t VertexMain(vs_input_t input)
{
    v2p_t v2p;
    float4 position = float4(input.localPosition, 1);
    float4 modelTransform = mul(ModelMatrix, position);
    
    
    float2 noiseInput = modelTransform.xy + float2(Time, 0);
    
    float noiseResult = customNoise(float3(noiseInput, 0));
    
    float baseHeight = -0.25f * input.color.b;
    float addeddHeight = input.color.b * 0.25f * noiseResult;
    modelTransform.z += float1(baseHeight + addeddHeight);
    
    float4 modelToViewPos = mul(ViewMatrix, modelTransform);
    v2p.position = mul(ProjectionMatrix, modelToViewPos);

    v2p.color = input.color;
    v2p.uv = input.uv;
    v2p.worldPosition = modelTransform;
    
    
    
    return v2p;
}