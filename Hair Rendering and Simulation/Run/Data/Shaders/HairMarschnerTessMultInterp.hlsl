#include "ShaderCommon.hlsl"


#if ENGINE_ANTIALIASING
Texture2DMS<float4> diffuseTexture : register(t0);
#else
Texture2D diffuseTexture : register(t0);
#endif
Texture2D noiseTexture : register(t1);
StructuredBuffer<HairData> HairInfo : register(t2);
SamplerState diffuseSampler : register(s0);

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
    vsOut.vId = input.vId;
    
    return vsOut;
}

// passthrough shader
[domain("isoline")]
[partitioning("fractional_odd")]
[outputtopology("line")]
[outputcontrolpoints(2)]
[patchconstantfunc("HullConstant")]
hull_out HullMain(InputPatch<vs_out, 2> inVertex, uint i : SV_OutputControlPointID)
{
    hull_out hullOut;
    
    hullOut.position = inVertex[i].position;
    hullOut.normal = inVertex[i].normal;
    hullOut.worldPosition = inVertex[i].worldPosition;
    hullOut.color = inVertex[i].color;
    hullOut.uv = inVertex[i].uv;
    hullOut.vId = inVertex[i].vId;
    
    return hullOut;
}
hull_tess_out HullConstant(InputPatch<vs_out, 2> patch)
{
    hull_tess_out tessOut;
    
    tessOut.EdgesTessellation[0] = float(InterpolationFactorMultiStrand);
    tessOut.EdgesTessellation[1] = TessellationFactor;
    
    return tessOut;
}

[domain("isoline")]
domain_out DomainMain(hull_tess_out input, float2 BarycentricCoords : SV_DomainLocation, OutputPatch<hull_out, 2> patch, uint patchID : SV_PrimitiveID)
{
    domain_out ds_out = (domain_out) 0;

    uint dimNoiseX = 0;
    uint dimPerlY = 0;
    uint SegmentCountNext = HairSegmentCount + 1;
    
    noiseTexture.GetDimensions(dimNoiseX, dimPerlY); // Y is unused
   
    uint dataIndex = (patchID) / HairSegmentCount;
    
    // For making HairSegments We need HairSegment + 1, e.g: for 12, we need 13 counting the end and beginning inclusive
    dataIndex *= SegmentCountNext * 3;
    
    uint dimHairBuffer = 0;
    uint stride = 0;
    HairInfo.GetDimensions(dimHairBuffer, stride);
    
    uint totalByte = (dimHairBuffer * stride);
    
    /*
    The index for getting the noise should be consistent between hair segments but different
    betwween hairs. BarycentricCoords.y will be the same between hair segments. DataIndex is different per hair
    */
    uint baseInd = dataIndex + uint(BarycentricCoords.y * float(dimNoiseX));
    uint noiseIndexX = baseInd % dimNoiseX;
    uint noiseIndexY = baseInd / dimNoiseX;
    
    uint firstHair = ((patchID) % HairSegmentCount) + dataIndex;
    uint secondHair = firstHair + SegmentCountNext;
    uint thirdHair = secondHair + SegmentCountNext;
    
    
    float4 hairOne = HairInfo[firstHair].Position;
    float4 hairTwo = HairInfo[secondHair].Position;
    float4 hairThree = HairInfo[thirdHair].Position;
    
    float4 secHairOne = HairInfo[firstHair + 1].Position;
    float4 secHairTwo = HairInfo[secondHair + 1].Position;
    float4 secHairThree = HairInfo[thirdHair + 1].Position;

    float4 prevHairOne = hairOne;
    float4 prevHairTwo = hairTwo;
    float4 prevHairThree = hairThree;
    
    float4 nextHairOne = secHairOne;
    float4 nextHairTwo = secHairTwo;
    float4 nextHairThree = secHairThree;
    
    if ((patchID % (HairSegmentCount)) != 0)
    {
        prevHairOne = HairInfo[firstHair - 1].Position;
        prevHairTwo = HairInfo[secondHair - 1].Position;
        prevHairThree = HairInfo[thirdHair - 1].Position;
        
       
    }
    
    if ((patchID % HairSegmentCount) != (HairSegmentCount - 1))
    {
        nextHairOne = HairInfo[firstHair + 2].Position;
        nextHairTwo = HairInfo[secondHair + 2].Position;
        nextHairThree = HairInfo[thirdHair + 2].Position;
    }
    
    
    float3 InterpolationWeights = noiseTexture[float2(noiseIndexX, noiseIndexY)].rgb;
    
        
    InterpolationWeights = smoothstep(0.0f.xxx, 1.0.xxx, InterpolationWeights);
    
    float total = InterpolationWeights.r + InterpolationWeights.g + InterpolationWeights.b;
    InterpolationWeights /= total;
    
    float3 InterpCoords = InterpolationWeights.xyz;

    float4 resultPrev = InterpCoords.x * prevHairOne +
                            InterpCoords.y * prevHairTwo +
                            InterpCoords.z * prevHairThree;
    
    float4 resultPositionOne = InterpCoords.x * hairOne +
                            InterpCoords.y * hairTwo +
                            InterpCoords.z * hairThree;
    
    float4 resultPositionTwo = InterpCoords.x * secHairOne +
                            InterpCoords.y * secHairTwo +
                            InterpCoords.z * secHairThree;
    
    
    float4 resultNext = InterpCoords.x * nextHairOne +
                            InterpCoords.y * nextHairTwo +
                            InterpCoords.z * nextHairThree;
    
    float3 prevPosition = resultPrev.xyz;
    float3 SegmentStart = resultPositionOne.xyz;
    float3 SegmentEnd = resultPositionTwo.xyz;
    float3 nextPosition = resultNext.xyz;
    
    float aThird = 1.0f / 3.0f;
    
    float3 a = prevPosition;
    float3 b = prevPosition + (SegmentEnd - SegmentStart) * aThird;
    float3 c = SegmentEnd - (nextPosition - SegmentEnd) * aThird;
    float3 d = SegmentEnd;
    
    //float t = BarycentricCoords.x * 0.5f;
    //t += 0.25f;
    
    float t = BarycentricCoords.x;
    float3 vWorldPos = EvaluateBezierCurve(t, a, b, c, d);
    
    //ds_out.worldPosition = lerp(resultPositionOne, resultPositionTwo, BarycentricCoords.x);
    ds_out.worldPosition = float4(vWorldPos, 1.0f);
    
    //ds_out.worldPosition = float4(HandleCollision(ds_out.worldPosition.xyz), 1.0f);
    ds_out.color = patch[0].color;
    ds_out.uv = patch[0].uv;
    
    return ds_out;
}


[maxvertexcount(4)]
void GeometryMain(line domain_out dsOut[2], inout TriangleStream<gs_out> triangleStream)
{
    float3 tangent = dsOut[1].worldPosition.xyz - dsOut[0].worldPosition.xyz;
    tangent = normalize(tangent);
    float3 iBasis = normalize(EyePosition - dsOut[0].worldPosition.xyz);
    //float3 jBasis = normalize(cross(float3(0.0f, 0.0f, 1.0f), iBasis));
    
    float3 jBasis = normalize(cross(iBasis, tangent));
    
    float4 vertexes[4];
    
    vertexes[0] = dsOut[0].worldPosition - (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[1] = dsOut[0].worldPosition + (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[2] = dsOut[1].worldPosition - (float4(jBasis, 0.0f) * HairWidth * 0.5f);
    vertexes[3] = dsOut[1].worldPosition + (float4(jBasis, 0.0f) * HairWidth * 0.5f);

    
    gs_out output = (gs_out) 0;
    
    
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        output.color = dsOut[0].color;
        output.worldPosition = vertexes[i];
        
        float4 viewPos = mul(ViewMatrix, vertexes[i]);
        output.position = mul(ProjectionMatrix, viewPos);
        output.uv = dsOut[0].uv;
        output.tangent = tangent;
        output.normal = dsOut[0].normal;
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
                totalDiffuse += ComputeDiffuseLighting(Lights[lightIndex].Position, input.worldPosition.xyz, input.tangent, DiffuseCoefficient, /*UseAcos,*/InvertLightDir);
                break;
            case SPOT_LIGHT:
                break;
        }
        
    }
    
    float4 resultingColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    [flatten]
    if (UseModelColor)
    {
        resultingColor = float4(totalSingle, 1.0f) + (totalDiffuse * ModelColor) + (AmbientIntensity * ModelColor);
    }
    else
    {
        resultingColor = float4(totalSingle, 1.0f) + (totalDiffuse * input.color * ModelColor) + (AmbientIntensity * input.color * ModelColor);
    }
        
    return resultingColor;
}

