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
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer LightConstants : register(b1)
{
    float3 DirectionalLight;
    float PaddingDirectionalLight;
    float4 DirectionalLightIntensity;
    float4 AmbientIntensity;
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

float4 PixelMain(v2p_t input) : SV_Target0
{
    float4 ambient = AmbientIntensity;
    float3 normalizedNormal = normalize(input.normal);
    float4 directional = DirectionalLightIntensity * saturate(dot(normalizedNormal, -DirectionalLight));
    float4 lightColor = ambient + directional;
    float4 diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    float4 resultingColor = lightColor * diffuseColor * input.color;
    
    clip(resultingColor.a - 0.5f);
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
    v2p.color = input.color;
    v2p.uv = input.uv;
    v2p.normal = normalWorldSpace;
    
    return v2p;
}