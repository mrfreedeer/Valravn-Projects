
struct ms_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
};

StructuredBuffer<ms_input_t> particlesInfo;

[numthreads(64,1,1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    ;
}