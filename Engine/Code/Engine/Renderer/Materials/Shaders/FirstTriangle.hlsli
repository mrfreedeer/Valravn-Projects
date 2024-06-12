//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct VSInput
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
    float depth : SV_TARGET1;
};

PSInput VertexMain(VSInput input)
{
    PSInput result;

    result.position = float4(input.localPosition, 1.0f);
    result.color = input.color;

    return result;
}

PSOutput PixelMain(PSInput input) : SV_TARGET
{
    PSOutput output = (PSOutput) 0;
    output.color = input.color;
    output.depth = 0.5f;
    return output;
}
