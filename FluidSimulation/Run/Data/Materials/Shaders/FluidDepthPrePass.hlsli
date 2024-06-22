#define MAX_LIGHTS 8
struct ms_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
};

struct ps_input_t
{
    float4 position : SV_Position;
    float3 eyeSpacePosition : POSITION0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct ps_output_t
{
    float worldDepth : SV_Depth;
};

struct Meshlet
{
    uint VertexCount;
    uint VertexOffset;
    uint PrimCount;
    uint PrimOffset;
    float4 Color;
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


cbuffer GameConstants : register(b3)
{
    float3 EyePosition;
    float SpriteRadius;
    float4 CameraUp;
    float4 CameraLeft;
}

Texture2D diffuseTexture : register(t0);
StructuredBuffer<ms_input_t> vertices : register(t1);
StructuredBuffer<Meshlet> meshlets : register(t2);
SamplerState diffuseSampler : register(s0);

[outputtopology("triangle")]
[numthreads(64, 1, 1)]
void MeshMain(
    in uint gtid : SV_GroupThreadID,
    in uint dtid : SV_DispatchThreadID,
    in uint gid : SV_GroupID,
    out vertices ps_input_t out_verts[256],
    out indices uint3 out_triangles[128])
{
    Meshlet meshlet = meshlets[gid];
    uint vertCount = meshlet.VertexCount * 4;
    uint primCount = meshlet.PrimCount * 2;
    
    SetMeshOutputCounts(vertCount, primCount);
    
    uint accessIndex = meshlet.VertexOffset + gtid;
    float3 position = vertices[accessIndex].localPosition;
    
    float3 jBasis = CameraLeft.xyz;
    float3 kBasis = CameraUp.xyz;
    float3 iBasis = normalize(EyePosition - position);

    uint genIndex = gtid * 4;
    
    if (genIndex < vertCount)
    {
        // Output a quad with UVs of (0.0f, 0.0f) - (1.0f, 1.0f) each
        
        float3 vertexPositions[4];
        vertexPositions[0] = position + SpriteRadius * (jBasis - kBasis); // Bottom Left
        vertexPositions[1] = position + SpriteRadius * -(jBasis + kBasis); // BottomRight
        vertexPositions[2] = position + SpriteRadius * (jBasis + kBasis); // Top Left
        vertexPositions[3] = position + SpriteRadius * (-jBasis + kBasis); // Top Right
        
        float2 UVs[4];
        UVs[0] = float2(0.0f, 0.0f);
        UVs[1] = float2(1.0f, 0.0f);
        UVs[2] = float2(0.0f, 1.0f);
        UVs[3] = float2(1.0f, 1.0f);
        
        for (int i = 0; i < 4; i++)
        {
            ps_input_t outVert = (ps_input_t) 0;
            float4 position = float4(vertexPositions[i], 1.0f);
            float4 viewPosition = mul(ViewMatrix, position);
            outVert.position = mul(ProjectionMatrix, viewPosition);
            outVert.eyeSpacePosition = viewPosition.xyz;
            outVert.color = vertices[accessIndex].color;
            outVert.uv = UVs[i];
        
            out_verts[genIndex + i] = outVert;

        }
    }
   
    if ((gtid * 2) < primCount)
    {
        out_triangles[gtid * 2] = uint3(genIndex, genIndex + 3, genIndex + 2);
        out_triangles[gtid * 2 + 1] = uint3(genIndex, genIndex + 1, genIndex + 3);
    }
    
    //SetMeshOutputCounts(4, 2);
    //if (gtid < 1)
    //{
        
    //    ps_input_t outVert = (ps_input_t) 0;
    //    Meshlet firstMeshlet = meshlets[0];
    //    uint numStructs = 0;
    //    uint stride = 0;
    //    meshlets.GetDimensions(numStructs, stride);
    //    outVert.position = float4(-1.0f, -1.0f, 1.0f, 1.0f);
    //    if (length(firstMeshlet.Color) == 0.0f)
    //    {
    //        if (length(ModelColor) == 0.0f)
    //        {
                
    //            if (stride == 0)
    //            {
    //                outVert.color = float4(1.0f, 0.1f, 1.0f, 1.0f);
    //            }
    //            else if (numStructs == 0)
    //            {
    //                outVert.color = float4(1.0f, 1.0f, 0.0f, 1.0f);
    //            }
    //            else
    //            {
    //                outVert.color = float4(0.0f, 0.0f, 1.0f, 1.0f);

    //            }
                
    //        }
    //        else
    //        {
    //            outVert.color = float4(0.0f, 1.0f, 0.0f, 1.0f);
    //        }
    //    }
    //    else
    //    {
    //        outVert.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    //    }
    //    //outVert.color = 0.0f.xxxx;
    //    outVert.uv = 0.0f.xx;
        
    //    out_verts[0] = outVert;
    //    outVert.position = float4(1.0f, -1.0f, 1.0f, 1.0f);
        
    //    outVert.uv = float2(1.0f, 0.0f);
        
    //    out_verts[1] = outVert;
    //    outVert.position = float4(1.0f, 1.0f, 1.0f, 1.0f);
    //    outVert.uv = float2(1.0f, 1.0f);
        
        
    //    out_verts[2] = outVert;
    //    outVert.position = float4(-1.0f, 1.0f, 1.0f, 1.0f);
    //    outVert.uv = float2(0.0f, 1.0f);
        
    //    out_verts[3] = outVert;
        
    //    out_triangles[0] = uint3(0, 2, 3);
    //    out_triangles[1] = uint3(0, 1, 2);
    //}
       
        
    
}

ps_output_t PixelMain(ps_input_t input)
{
    ps_output_t output = (ps_output_t) 0;
    
    float2 posInCircle = (input.uv * 2.0f) - 1.0f;
    float radiusSqr = dot(posInCircle, posInCircle);
    
    if (radiusSqr > 1.0f)
        discard;
    
    // The normal is simulated to a the normals in a sphere. In the center
    // It will be only the -forward component, as it's pointing towards the camera
    // At the edges, the normal will be the uv positions.
    float3 normal;
    normal.yz= posInCircle;
    normal.y *= -1.0f;
    normal.x = -sqrt(1.0f - radiusSqr);
    float4 eyeSpaceNormalPoint = float4(normal * SpriteRadius + input.eyeSpacePosition, 1.0f);
    float4 clipPos = mul(ProjectionMatrix, eyeSpaceNormalPoint);
    float depth = clipPos.z / clipPos.w;
    
    //float4 resultingColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * ModelColor;
    
    float4 resultingColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * meshlets[0].Color * saturate(dot(normal, Lights[0].Direction));
    //if (resultingColor.w == 0)
    //    discard;
    
    output.worldDepth = depth;
    return output;
}
