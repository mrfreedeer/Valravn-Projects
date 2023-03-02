#include "ShaderCommon.hlsl"

//Texture2DMS<float4> diffuseTexture : register(t0);

#if ENGINE_ANTIALIASING
Texture2DMS<float4> diffuseTexture : register(t0);
#else
Texture2D diffuseTexture : register(t0);
#endif

SamplerState diffuseSampler : register(s0);
StructuredBuffer<HairData> HairInfo : register(t2);

// passthrough shader
[domain("isoline")]
[partitioning("fractional_odd")]
[outputtopology("line")]
[outputcontrolpoints(2)]
[patchconstantfunc("HullConstant")]
hull_out HullMain(InputPatch<vs_out, 2> inVertex, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    int baseIndex = patchId / HairSegmentCount;
    int hairSegment = patchId % HairSegmentCount;
    baseIndex *= (HairSegmentCount + 1);
    int hairIndex = baseIndex + hairSegment;
    //int hairDataIndex = patchId * 2; 
    
    int accessIndex = hairIndex + i;
    
    hull_out hullOut;
    
    
    hullOut.position = inVertex[i].position;
    hullOut.normal = inVertex[i].normal;
    //hullOut.worldPosition = inVertex[i].worldPosition;
    hullOut.previousWorldPosition = HairInfo[hairIndex].Position;
    hullOut.worldPosition = HairInfo[accessIndex].Position;
    hullOut.nextWorldPostion = HairInfo[hairIndex + 2].Position;
    hullOut.color = inVertex[i].color;
    hullOut.uv = inVertex[i].uv;
    
    if (hairSegment == 0)
    {
        hullOut.previousWorldPosition = hullOut.worldPosition;;
    }
    
    if (hairSegment == (HairSegmentCount - 1))
    {
        hullOut.nextWorldPostion = hullOut.worldPosition;
    }
    
    return hullOut;
}
hull_tess_out HullConstant(InputPatch<vs_out, 2> patch, uint patchID : SV_PrimitiveID)
{
    hull_tess_out tessOut;
    
    tessOut.EdgesTessellation[0] = InterpolationFactor;
    tessOut.EdgesTessellation[1] = TessellationFactor;
    
    return tessOut;
}

[domain("isoline")]
domain_out DomainMain(hull_tess_out input, float2 BarycentricCoords : SV_DomainLocation, OutputPatch<hull_out, 2> patch)
{
    domain_out ds_out;
    
    float3 dispToEye = EyePosition - patch[0].worldPosition.xyz;
    static float startX = CustomNoise(StartTime);
    static float startY = CustomNoise(StartTime + 1.0f);
    static float startZ = CustomNoise(StartTime + 2.0f);
    
    static float3 StartVector = float3(startX, startY, startZ);
    
    float3 randDir = normalize(cross(StartVector, patch[0].normal.xyz));
    
    
    float3 offset = float3(0.0f, 0.0f, 0.0f);
    float rawNoise = CustomNoise(BarycentricCoords.y);
    float randNoise = 0.5f * (rawNoise + 1.0f);
    
    float randRotation = randNoise * 360.0f;
    float3 rotatedDir = RotateVectorAroundAxis(randDir, randRotation, patch[0].normal);
    
    float maxOffest = InterpolationRadius;
    offset = (maxOffest * (randNoise + 0.1f)) * rotatedDir; // Creates offset in a rand radius, 0.1 at least so hair doesn't overlap
        
    if (!InterpolateUsingRadius)
    {
        /*
        If we have the tanget of the hair segment, and the normal of the scalp, then what we want is
        the other vector that forms this basis
        */
        float3 tangent = normalize(patch[1].worldPosition - patch[0].worldPosition);
        float3 jBasis = normalize(cross(tangent, patch[0].normal));
        rotatedDir = jBasis;

        offset = (maxOffest * (randNoise + 0.1f)) * sign(rawNoise) * rotatedDir;
    }
    
    
    float3 currentOffset = lerp(float3(0.0f, 0.0f, 0.0f), offset, BarycentricCoords.y);
    //float3 vWorldPos = lerp(patch[0].worldPosition, patch[1].worldPosition, BarycentricCoords.x).xyz + currentOffset;
    
    float3 prevPosition = patch[0].previousWorldPosition;
    float3 SegmentStart = patch[0].worldPosition;
    float3 SegmentEnd = patch[1].worldPosition;
    float3 nextPosition = patch[1].nextWorldPostion;
   
    /* 
    Bezier Curve Control Points need to be computed from the positions
    Just using the world points as Control creates a disjointed curve, 
    because the control points are actually outside of the hair curve!
    */
    float aThird = 1.0f / 3.0f;
    
    float3 a = prevPosition;
    float3 b = prevPosition + (SegmentEnd - SegmentStart) * aThird;
    float3 c = SegmentEnd - (nextPosition - SegmentEnd) * aThird;
    float3 d = SegmentEnd;
    
    //float t = BarycentricCoords.x * 0.5f;
    //t += 0.25f;
    
    float t = BarycentricCoords.x;
    float3 vWorldPos = EvaluateBezierCurve(t, a, b, c, d) + currentOffset;
    
    
    ds_out.worldPosition = float4(vWorldPos, 1.0f);
    ds_out.color = patch[0].color;
    ds_out.uv = patch[1].uv;
    
    return ds_out;
}


[maxvertexcount(4)]
void GeometryMain(line domain_out dsOut[2], inout TriangleStream<gs_out> triangleStream)
{
    //int baseIndex = primitiveID / HairSegmentCount;
    //int hairSegment = primitiveID % HairSegmentCount;
    //baseIndex *= (HairSegmentCount + 1);
    //int hairIndex = baseIndex + hairSegment;
    //int hairDataIndex = primitiveID * 2; //
    
    //dsOut[0].worldPosition = HairInfo[hairIndex].Position;
    //dsOut[1].worldPosition = HairInfo[hairIndex + 1].Position;

    float3 iBasis = normalize(EyePosition - dsOut[0].worldPosition.xyz);
    float3 jBasis = normalize(cross(float3(0.0f, 0.0f, 1.0f), iBasis));
    
    
    float4 vertexes[4];
    
    vertexes[0] = dsOut[0].worldPosition - (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[1] = dsOut[0].worldPosition + (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[2] = dsOut[1].worldPosition - (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[3] = dsOut[1].worldPosition + (float4(jBasis, 0.0f) * HairWidth * 0.5f);

    
    gs_out output = (gs_out) 0;
    
    float3 tangent = dsOut[1].worldPosition.xyz - dsOut[0].worldPosition.xyz;
    
    tangent = normalize(tangent);
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        output.color = dsOut[0].color;
        output.worldPosition = vertexes[i];
        
        float4 viewPos = mul(ViewMatrix, vertexes[i]);
        output.position = mul(ProjectionMatrix, viewPos);
        output.uv = dsOut[0].uv;
        output.tangent = tangent;
        
        triangleStream.Append(output);
    }
    
}






float4 PixelMain(gs_out input) : SV_Target0
{
    float3 totalSingle = 0.0f;
    float totalDiffuse = 0.0f;
    
    float4 diffuseColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
#if ENGINE_ANTIALIASING
   uint antialiasingLevel = ANTIALIASING_LEVEL;
    for (uint sampleIndex = 0; sampleIndex < antialiasingLevel; sampleIndex++)
    {
       diffuseColor += diffuseTexture.Load(input.uv, sampleIndex);
    }
    
    diffuseColor /= antialiasingLevel;
    diffuseColor *= input.color * ModelColor;
#else
    diffuseColor = diffuseTexture.Sample(diffuseSampler, input.uv) * input.color * ModelColor;
#endif
 
    //if (diffuseColor.w == 0)
    //    discard;
    
    [unroll]

    for (int lightIndex = 0; lightIndex < MAX_LIGHTS; lightIndex++)
    {
        if (!Lights[lightIndex].Enabled)
            continue;
        switch (Lights[lightIndex].LightType)
        {
            case POINT_LIGHT:
                totalSingle += ComputeSingleScattering(Lights[lightIndex], input);
                totalDiffuse += ComputeDiffuseLighting(Lights[lightIndex].Position, input.worldPosition.xyz, input.tangent, DiffuseCoefficient);
                break;
            case SPOT_LIGHT:
                break;
        }
        
    }
    
        
    float4 resultingColor = float4(totalSingle, 1.0f) + (totalDiffuse * ModelColor) + (AmbientIntensity * ModelColor);
    
    return resultingColor;
}


vs_out VertexMain(vs_input input)
{
    vs_out vsOut;
    float4 position = float4(input.localPosition, 1);
    float4 normal = float4(input.normal, 0);
    
    float4 modelTransform = mul(ModelMatrix, position);
    float4 modelToViewPos = mul(ViewMatrix, modelTransform);
    
    vsOut.position = mul(ProjectionMatrix, modelToViewPos);
    vsOut.worldPosition = modelTransform;
    vsOut.color = input.color;
    vsOut.uv = input.uv;
    vsOut.normal = input.normal;
    
    return vsOut;
}