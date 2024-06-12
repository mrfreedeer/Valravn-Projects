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
SamplerState SurfaceSampler : register(s0);

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
    float4 resultingColor = DiffuseTexture.Sample(SurfaceSampler, input.uv);
    
    return 1.0f.xxxx - resultingColor;
}