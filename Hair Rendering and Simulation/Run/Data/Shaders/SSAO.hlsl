#include "Constants.hlsl"


// input to the vertex shader - for now, a special input that is the index of the vertex we're drawing
struct vs_input_t
{
    uint vidx : SV_VERTEXID; // SV_* stands for System Variable (ie, built-in by D3D11 and has special meaning)
                           // in this case, is the the index of the vertex coming in.
};

// Output from Vertex Shader, and Input to Fragment Shader
// For now, just position
struct VertexToFragment_t
{
    float4 position : SV_POSITION;
    float2 uv : TEX_COORD;
};

//#if ENGINE_ANTIALIASING
//Texture2DMS<float4> DiffuseTexture : register(t0);
//Texture2DMS<float4> DepthBuffer : register(t1);
//#else
Texture2D DiffuseTexture : register(t0);
Texture2D<float4> DepthBuffer : register(t1);
//#endif
Texture2D NoiseTexture : register(t2);
StructuredBuffer<float4> Samples : register(t3);
SamplerState SurfaceSampler : register(s0);

float SampleDepthBufferUsingPosition(float2 texPosition)
{
//#if ENGINE_ANTIALIASING
//    uint texWidth = 0;
//    uint texHeight = 0;
//    uint aliasingLevel =0;
//    DiffuseTexture.GetDimensions(texWidth, texHeight, aliasingLevel);
//    float totalSample = 0.0f;
//    for(uint sampleIndex = 0; sampleIndex < aliasingLevel; sampleIndex++){
//        totalSample += DepthBuffer.Load(texPosition, sampleIndex).r;
//    }
    
//    totalSample /= aliasingLevel;
//    return totalSample;
//#else
    uint texWidth = 0;
    uint texHeight = 0;
    uint aliasingLevel = 0;
    DiffuseTexture.GetDimensions(texWidth, texHeight);
    
    return DepthBuffer.Sample(SurfaceSampler, texPosition / float2(texWidth, texHeight)).r;
//#endif
}

float SampleDepthBuffer(float2 uv)
{
//#if ENGINE_ANTIALIASING
//    uint texWidth = 0;
//    uint texHeight = 0;
//    uint aliasingLevel =0;
//    DiffuseTexture.GetDimensions(texWidth, texHeight, aliasingLevel);
//    float totalSample = 0.0f;
//    float2 pos = uv * float2(texWidth, texHeight);
//    for(uint sampleIndex = 0; sampleIndex < aliasingLevel; sampleIndex++){
//        totalSample += DepthBuffer.Load(pos, sampleIndex).r;
//    }
    
//    totalSample /= aliasingLevel;
//    return totalSample;
//#else
    return DepthBuffer.Sample(SurfaceSampler, uv).r;
//#endif
}

float3 GetScreenSpaceNormal(float depth, float2 texcoords)
{
//#if ENGINE_ANTIALIASING
//     uint texWidth = 0;
//    uint texHeight = 0;
//    uint aliasingLevel =0;
//    DiffuseTexture.GetDimensions(texWidth, texHeight, aliasingLevel);
//    float2 sampleDeltaXY = float2(1.5f, 1.5f) / (float2(texWidth, texHeight));
//    float2 offsetX = float2(sampleDeltaXY.x, 0.0f);
//    float2 offsetY = float2(0.0f, sampleDeltaXY.x);
    
//#else
    static const float sampleDelta = 0.000001f;
    static const float2 offsetY = float2(0.0, sampleDelta);
    static const float2 offsetX = float2(sampleDelta, 0.0);
//#endif

  
    float rightDepth = SampleDepthBuffer(texcoords + offsetX);
    float upDepth = SampleDepthBuffer(texcoords + offsetY);
    float leftDepth = SampleDepthBuffer(texcoords - offsetX);
    float downDepth = SampleDepthBuffer(texcoords - offsetY);
  
    float3 rightBasis = float3(0.0f, 1.0f, rightDepth - depth);
    float3 upBasis = float3(1.0f, 0.0f, upDepth - depth);
    float3 leftBasis = float3(0.0f, -1.0f, leftDepth - depth);
    float3 downBasis = float3(-1.0f, 0.0f, downDepth - depth);
  
    float3 normal1 = cross(upBasis, rightBasis);
    float3 normal2 = cross(leftBasis, upBasis);
    float3 normal3 = cross(downBasis, leftBasis);
    float3 normal4 = cross(rightBasis, downBasis);
    
    
    float3 resultingNormal = normal1 + normal2 + normal3 + normal4;
    resultingNormal *= 0.25f;

    return normalize(resultingNormal);
}

//--------------------------------------------------------------------------------------
// constants
//--------------------------------------------------------------------------------------
// The term 'static' refers to this an built into the shader, and not coming
// from a constant buffer - which we'll get into later (if you remove static, you'll notice
// this stops working). 
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

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VertexToFragment_t VertexMain(vs_input_t input)
{
    VertexToFragment_t v2f;
    
    v2f.position = float4(FS_POS[input.vidx], 1.0f);
    v2f.uv = FS_UVS[input.vidx];

    return v2f;
}

float4 PixelMain(VertexToFragment_t input) : SV_Target0 // semeantic of what I'm returning
{
    float3 noise = NoiseTexture.Sample(SurfaceSampler, input.uv * 4.0f).rgb;
    float3 random = normalize(noise);
  
    float depth = 0.0f;
    float3 diffuse = float3(0.0f, 0.0f, 0.0f);
    float3 position = float3(0.0f, 0.0f, 0.0f);
    

    
//#if ENGINE_ANTIALIASING
//    uint texWidth = 0;
//    uint texHeight = 0;
//    uint aliasingLevel =0;
//    DiffuseTexture.GetDimensions(texWidth, texHeight, aliasingLevel);
    
//    position = float3(input.uv * float2(texWidth, texHeight), 0.0f);
    
//    for (uint sampleIndex = 0; sampleIndex < aliasingLevel; sampleIndex++)
//    {
//        diffuse += DiffuseTexture.Load(position.xy , sampleIndex).rgb;
//        depth += DepthBuffer.Load(position.xy, sampleIndex).r;
//    }
    
//    diffuse /= aliasingLevel;
//    depth /= aliasingLevel;
    
//    position.z = depth;
//#else
    diffuse = DiffuseTexture.Sample(SurfaceSampler, input.uv).rgb;
    depth = DepthBuffer.Sample(SurfaceSampler, input.uv).r;
    position = float3(input.uv, depth);
//#endif
    float3 normal = GetScreenSpaceNormal(depth, input.uv);
  

    float radiusScaledByDepth = SampleRadius / depth;
    float occlusion = 0.0;
    for (int i = 0; i < SampleSize; i++)
    {
        float3 ray = radiusScaledByDepth * normalize(noise.rgb) * Samples[i].xyz;
        float3 samplePosition = float3(0.0f, 0.0f, 0.0f);
        float sampleDepth = 0.0f;
//#if ENGINE_ANTIALIASING
//        ray =  5.0f * normalize(noise.rgb) * Samples[i].xyz;
//        float3 rayEndToAdd = sign(dot(ray, normal)) * ray;
//        //rayEndToAdd = normalize(rayEndToAdd);
//        //rayEndToAdd.xy *= float2(texWidth, texHeight);
        
//        samplePosition = position + rayEndToAdd;
//        sampleDepth = SampleDepthBufferUsingPosition(samplePosition.xy);
        
//#else 
        samplePosition = position + sign(dot(ray, normal)) * ray;
        sampleDepth = DepthBuffer.Sample(SurfaceSampler, samplePosition.xy).r;;
//#endif
        float difference = depth - sampleDepth;
        float depthPastThreshold = step(SSAOFalloff, difference);
        float smoothStepRes = smoothstep(SSAOFalloff, MaxOcclusionPerSample, difference);
        
        occlusion += depthPastThreshold * (1.0 - smoothStepRes);
    }
  
    float resultingLighting = AmbientIntensity + 1.0f - (occlusion / SampleSize);
    resultingLighting = saturate(resultingLighting);
    
       
    //return resultingLighting.xxxx;
    return float4(diffuse * resultingLighting, 1.0f);
}