#define MAX_LIGHTS 8
struct ms_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
};

struct ps_input_t
{
    float4 position : SV_Position;
    float3 normal : normal;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct Meshlet
{
    uint VertexCount;
    uint VertexOffset;
    uint PrimCount;
    uint PrimOffset;
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
    in uint dtid: SV_DispatchThreadID,
    in uint gid: SV_GroupID,
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
        float3 vertexPositions[4];
        vertexPositions[0] = position + SpriteRadius * (jBasis - kBasis); // Bottom Left
        vertexPositions[1] = position + SpriteRadius * -(jBasis + kBasis); // BottomRight
        vertexPositions[2] = position + SpriteRadius * (jBasis + kBasis); // Top Left
        vertexPositions[3] = position + SpriteRadius * (-jBasis + kBasis); // Top Right
        for (int i = 0; i < 4; i++)
        {
            ps_input_t outVert = (ps_input_t) 0;
            float4 position = float4(vertexPositions[i], 1.0f);
            float4 viewPosition = mul(ViewMatrix, position);
            outVert.position = mul(ProjectionMatrix, viewPosition);
            outVert.normal = iBasis; // poiting towards camera
            outVert.color = vertices[accessIndex].color;
            outVert.uv = 0.0f.xx;
        
            out_verts[genIndex + i] = outVert;

        }
    }
   
    if ((gtid * 2) < primCount)
    {
        out_triangles[gtid * 2] = uint3(genIndex, genIndex + 3, genIndex + 2);
        out_triangles[gtid * 2 + 1] = uint3(genIndex, genIndex + 1, genIndex + 3);
    }
    
    
        //out_triangles[tid * 2] = uint3(genIndex, genIndex + 1, genIndex + 2);
        //out_triangles[tid * 2 + 1] = uint3(genIndex, genIndex + 2, genIndex + 3);
        
    
}

float4 PixelMain(ps_input_t input) : SV_Target0
{
    //float4 ambient = AmbientIntensity;
    //float3 normalizedNormal = normalize(input.normal);
    //float4 directional = DirectionalLightIntensity * saturate(dot(normalizedNormal, -DirectionalLight));
    //float4 lightColor = ambient + directional + ComputeDiffuseLighting(input.worldPosition, normalizedNormal);
    //float4 diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    //float4 resultingColor = lightColor * diffuseColor * input.color;
    
    //clip(resultingColor.a - 0.5f);
    
    float4 resultingColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * ModelColor;
    if (resultingColor.w == 0)
        discard;
    
    return resultingColor;
}
