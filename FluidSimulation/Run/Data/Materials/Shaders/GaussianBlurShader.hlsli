#include "GaussianBlurLib.hlsli"
Texture2D<float> inputTexture : register(t0);

cbuffer BlurConstants : register(b4)
{
    float KernelRadius;
    float GaussianSigma;
    float ClearValue;
    // b01 for vertical
    // b10 for horizontal
    // b100 check value <= clearvalue  -> Depth
    // b1000 check value >= clearvalue -> Thickness
    unsigned int DirectionFlags;
    row_major float4x4 GaussianKernelsPacked;
}
    

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

ps_input_t VertexMain(vs_input_t input)
{
    ps_input_t output = (ps_input_t) 0;
    
    output.position = float4(FS_POS[input.vidx], 1.0f);
    output.uv = FS_UVS[input.vidx];
    
    return output;
}

float PixelMain(ps_input_t input) : SV_Target0
{
    uint2 texDims;
    inputTexture.GetDimensions(texDims.x, texDims.y);
    const float2 epsilon = (1.0f.xx / float2(texDims));
    const unsigned int horizontalMultiplier = (DirectionFlags & (1 << 1)) != 0;
    const unsigned int verticalMultiplier = (DirectionFlags & (1 << 0)) != 0;
    
    const unsigned int checkLessThanMultiplier = (DirectionFlags & (1 << 2)) != 0;
    const unsigned int checkGreaterThanMultiplier = (DirectionFlags & (1 << 3)) != 0;
    
    const float2 sampleDelta = epsilon * float2(float(horizontalMultiplier), float(verticalMultiplier));
    static const float GaussianKernels[16] = (float[16])(GaussianKernelsPacked);
    
    unsigned int kernelRadiusUINT = (unsigned int) (KernelRadius);
    
    
    float texValue = inputTexture.Sample(diffuseSampler, input.uv);
    float outBlurredVal =  texValue * GaussianKernels[0];
    float greaterThanComp = (texValue > ClearValue) ? float(checkGreaterThanMultiplier) : 0.0f; // TO ALLOW >= 0 for thickness textures, as it is cleared to 0;
    float lessThanComp = (texValue < ClearValue) ? float(checkLessThanMultiplier) : 0.0f; // TO ALLOW <= 0 for depth textures, as it is cleared to 1.0f;
    
    if ((greaterThanComp + lessThanComp) < 1.0f)
        return ClearValue;
    
    //if (outBlurredVal >= ClearValue)
    //    return 0.0f;
    
    
    for (unsigned int kernelInd = 1; kernelInd <= kernelRadiusUINT; kernelInd++)
    {
        float upValue = inputTexture.Sample(diffuseSampler, input.uv + (sampleDelta * kernelInd));
        float downValue = inputTexture.Sample(diffuseSampler, input.uv - (sampleDelta * kernelInd));
        float weight = GaussianKernels[kernelInd];
        outBlurredVal +=  upValue * weight;
        outBlurredVal += downValue * weight;
    }
    
    return outBlurredVal;
}
