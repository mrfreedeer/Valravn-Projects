#include "Constants.hlsli"

Texture2D diffuseTexture : register(t0);
StructuredBuffer<Particle> particles : register(t1);
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
    Particle currentParticle = particles[dtid.x];
    
    uint vertCount = 256;
    uint primCount = 128;
    
    SetMeshOutputCounts(vertCount, primCount);
    
    float3 position = currentParticle.Position;
    
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
            outVert.color = ParticleColor;
            outVert.uv = UVs[i];
        
            out_verts[genIndex + i] = outVert;

        }
    }
   
    if ((gtid * 2) < primCount)
    {
        out_triangles[gtid * 2] = uint3(genIndex, genIndex + 3, genIndex + 2);
        out_triangles[gtid * 2 + 1] = uint3(genIndex, genIndex + 1, genIndex + 3);
    }
}