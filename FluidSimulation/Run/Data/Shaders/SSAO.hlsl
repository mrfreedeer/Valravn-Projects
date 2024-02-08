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

Texture2D<float4> DiffuseTexture : register(t0);
Texture2D<float4> DepthBuffer : register(t1);
Texture2D NoiseTexture : register(t2);
StructuredBuffer<float4> Samples : register(t3);
SamplerState SurfaceSampler : register(s0);

float3 GetScreenSpaceNormal(float depth, float2 texcoords)
{
    static const float sampleDelta = 0.000001f;
    
    static const float2 offsetY = float2(0.0, sampleDelta);
    static const float2 offsetX = float2(sampleDelta, 0.0);
  
    float rightDepth = DepthBuffer.Sample(SurfaceSampler, texcoords + offsetX).r;
    float upDepth = DepthBuffer.Sample(SurfaceSampler, texcoords + offsetY).r;
    float leftDepth = DepthBuffer.Sample(SurfaceSampler, texcoords - offsetX).r;
    float downDepth = DepthBuffer.Sample(SurfaceSampler, texcoords - offsetY).r;
  
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
    //const int samples = 16;
    //float3 sample_sphere[samples] =
    //{
    //    float3(0.5381, 0.1856, -0.4319), float3(0.1379, 0.2486, 0.4430),
    //  float3(0.3371, 0.5679, -0.0057), float3(-0.6999, -0.0451, -0.0019),
    //  float3(0.0689, -0.1598, -0.8547), float3(0.0560, 0.0069, -0.1843),
    //  float3(-0.0146, 0.1402, 0.0762), float3(0.0100, -0.1924, -0.0344),
    //  float3(-0.3577, -0.5301, -0.4358), float3(-0.3169, 0.1063, 0.0158),
    //  float3(0.0103, -0.5869, 0.0046), float3(-0.0897, -0.4940, 0.3287),
    //  float3(0.7119, -0.0154, -0.0918), float3(-0.0533, 0.0596, -0.5411),
    //  float3(0.0352, -0.0631, 0.5460), float3(-0.4776, 0.2847, -0.0271)
    //};
    
    //float3 sample_sphere[samples] =
    //{
    //    float3(0.5381, 0.1856, 0.4319), float3(0.1379, 0.2486, 0.4430),
    //  float3(0.3371, 0.5679, 0.0057), float3(-0.6999, -0.0451, 0.0019),
    //  float3(0.0689, -0.1598, 0.8547), float3(0.0560, 0.0069, 0.1843),
    //  float3(-0.0146, 0.1402, 0.0762), float3(0.0100, -0.1924, 0.0344),
    //  float3(-0.3577, -0.5301, 0.4358), float3(-0.3169, 0.1063, 0.0158),
    //  float3(0.0103, -0.5869, 0.0046), float3(-0.0897, -0.4940, 0.3287),
    //  float3(0.7119, -0.0154, 0.0918), float3(-0.0533, 0.0596, 0.5411),
    //  float3(0.0352, -0.0631, 0.5460), float3(-0.4776, 0.2847, 0.0271)
    //};
    
    float3 noise = NoiseTexture.Sample(SurfaceSampler, input.uv * 4.0f).rgb;
    float3 random = normalize(noise);
  
    float depth = DepthBuffer.Sample(SurfaceSampler, input.uv).r;
 
    float3 position = float3(input.uv, depth);
    float3 normal = GetScreenSpaceNormal(depth,input.uv);
  
    //float radius_depth = radius / depth;
    float radiusScaledByDepth = SampleRadius / depth;
    float occlusion = 0.0;
    for (int i = 0; i < SampleSize; i++)
    {
        float3 ray = radiusScaledByDepth * normalize(noise.rgb) * Samples[i];
        float3 samplePosition = position + sign(dot(ray, normal)) * ray;
        
        float sampleDepth = DepthBuffer.Sample(SurfaceSampler, saturate(samplePosition.xy)).r;
        float difference = depth - sampleDepth;
        float depthPastThreshold = step(SSAOFalloff, difference);
        float smoothStepRes = smoothstep(SSAOFalloff, MaxOcclusionPerSample, difference);
        
        occlusion += depthPastThreshold * (1.0 - smoothStepRes);
    }
  
    float resultingLighting = AmbientIntensity +  1.0f - (occlusion / SampleSize);
    resultingLighting = saturate(resultingLighting);
    
    float3 diffuse = DiffuseTexture.Sample(SurfaceSampler, input.uv).rgb;
       
    //return resultingAO.xxxx;
    return float4(diffuse * resultingLighting, 1.0f);
}