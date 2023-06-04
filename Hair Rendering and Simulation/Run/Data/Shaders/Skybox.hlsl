struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_Position;
    float3 worldPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

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

TextureCube diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

float4 PixelMain(v2p_t input) : SV_Target0
{
    return diffuseTexture.Sample(diffuseSampler, input.worldPosition.xyz);
}


v2p_t VertexMain(vs_input_t input)
{
    v2p_t v2p;
    float4 position = float4(input.localPosition, 1);
    float4 modelTransform = mul(ModelMatrix, position);
    float4 modelToViewPos = mul(ViewMatrix, modelTransform);
    v2p.position = mul(ProjectionMatrix, modelToViewPos);
    v2p.worldPosition = modelTransform.xyz;
    v2p.position.z = v2p.position.w;
    v2p.color = input.color;
    v2p.uv = input.uv;
    return v2p;
}