#include "LightOnlyCommon.hlsl"

#if ENGINE_ANTIALIASING
Texture2DMS<float4> diffuseTexture : register(t0);
#else
Texture2D diffuseTexture : register(t0);
#endif
SamplerState diffuseSampler : register(s0);
StructuredBuffer<HairData> HairInfo : register(t2);
uint antialiasingLevel = ANTIALIASING_LEVEL;
v2g_t VertexMain(vs_input_t input)
{
    v2g_t outputVertex;
    
    float4 position = float4(input.localPosition, 1);
    float4 modelTransform = mul(ModelMatrix, position);
    outputVertex.worldPosition = modelTransform;
    
    float4 modelToViewPos = mul(ViewMatrix, modelTransform);
    outputVertex.position = mul(ProjectionMatrix, modelToViewPos);

    outputVertex.color = input.color;
    outputVertex.uv = input.uv;
    return outputVertex;
}

[maxvertexcount(4)]
void GeometryMain(line v2g_t geoInput[2], inout TriangleStream<g2p_t> triangleStream, uint primitiveID: SV_PrimitiveID)
{    
    int baseIndex = primitiveID / HairSegmentCount;
    int hairSegment = primitiveID % HairSegmentCount;
    baseIndex *= (HairSegmentCount + 1);
    int hairIndex = baseIndex + hairSegment;
    //int hairDataIndex = primitiveID * 2; //
    
    geoInput[0].worldPosition = HairInfo[hairIndex].Position;
    geoInput[1].worldPosition = HairInfo[hairIndex + 1].Position;
    
    float3 iBasis = normalize(EyePosition - geoInput[0].worldPosition.xyz);
    float3 jBasis = normalize(cross(float3(0.0f, 0.0f, 1.0f), iBasis));
    
    
    float4 vertexes[4];
    
    vertexes[0] = geoInput[0].worldPosition - (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[1] = geoInput[0].worldPosition + (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[2] = geoInput[1].worldPosition - (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[3] = geoInput[1].worldPosition + (float4(jBasis, 0.0f) * HairWidth * 0.5f);

    
    g2p_t output = (g2p_t) 0;
    
    float3 tangent = geoInput[1].worldPosition.xyz - geoInput[0].worldPosition.xyz;
    
    tangent = normalize(tangent);
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        output.color = geoInput[0].color;
        output.worldPosition = vertexes[i];
        
        float4 viewPos = mul(ViewMatrix, vertexes[i]);
        output.position = mul(ProjectionMatrix, viewPos);
        output.uv = geoInput[0].uv;
        output.tangent = tangent;
        
        triangleStream.Append(output);
    }
    
}

float4 PixelMain(g2p_t input) : SV_Target0
{
    float totalDiffuse = 0.0f;
    float totalSpecular = 0.0f;
    
    float4 diffuseColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
#if ENGINE_ANTIALIASING
   
    for (uint sampleIndex = 0; sampleIndex <antialiasingLevel; sampleIndex++)
    {
       diffuseColor += diffuseTexture.Load(input.uv, sampleIndex);
    }
    
    diffuseColor /= antialiasingLevel;
    diffuseColor *= input.color * ModelColor;
#else
    diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * ModelColor;
#endif
    //if (diffuseColor.w == 0)
    //    discard;
    
    [unroll]

    for (int lightIndex = 0; lightIndex < MAX_LIGHTS; lightIndex++)
    {
        if (!Lights[lightIndex].Enabled)
            continue;
        switch (Lights[lightIndex].LightType)
        {
            case POINT_LIGHT:
                totalDiffuse += ComputeDiffuseLighting(Lights[lightIndex].Position, input.worldPosition.xyz, input.tangent, DiffuseCoefficient, /*UseAcos,*/ InvertLightDir);
                totalSpecular += ComputeSpecularLighting(Lights[lightIndex].Position, input.worldPosition.xyz, EyePosition, input.tangent, SpecularExponent, SpecularCoefficient);
                break;
            case SPOT_LIGHT:
                break;
        }
        
    }
    
        
    totalDiffuse = saturate(totalDiffuse);
    totalSpecular = saturate(totalSpecular);
    float4 resultingColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    [flatten]
    if (UseModelColor)
    {
        resultingColor = (totalSpecular + totalDiffuse + AmbientIntensity.r) * ModelColor;
    }
    else
    {
        resultingColor = (totalSpecular + totalDiffuse + AmbientIntensity.r) * input.color * ModelColor;

    }
    
    return resultingColor;
}


