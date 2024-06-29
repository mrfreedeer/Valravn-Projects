
RWTexture2D <float> outBlurTex : register(u0);

[numthreads(64,1,1)]
void ComputeMain(uint3 threadId : SV_DispatchThreadID)
{
    outBlurTex[threadId.xy] = 27.0f;
}