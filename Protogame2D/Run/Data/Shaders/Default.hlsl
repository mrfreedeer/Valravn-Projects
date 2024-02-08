struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

struct v2p_t
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};

cbuffer CameraConstants : register(b2) {
	float1 OrthoMinX;
	float1 OrthoMinY;
	float1 OrthoMinZ;
	float1 PaddingMin;
	float1 OrthoMaxX;
	float1 OrthoMaxY;
	float1 OrthoMaxZ;
	float1 PaddingMax;
};

Texture2D diffuseTexture: register(t0);
SamplerState diffuseSampler: register(s0);

float1 GetFractionWithin(float1 inValue, float1 inStart, float1 inEnd)
{
	if (inStart == inEnd) return 0.5f;
	float1 range = inEnd - inStart;
	return (inValue - inStart) / range;
}

float1 Interpolate(float1 outStart, float1 outEnd, float1 fraction)
{
	return outStart + fraction * (outEnd - outStart);
}

float1 RangeMap(float1 inValue, float1 inStart, float1 inEnd, float1 outStart, float1 outEnd)
{
	float1 fraction = GetFractionWithin(inValue, inStart, inEnd);
	return Interpolate(outStart, outEnd, fraction);
}

float4 PixelMain(v2p_t input) : SV_Target0
{
	return diffuseTexture.Sample(diffuseSampler, input.uv) * input.color;
}


v2p_t VertexMain(vs_input_t input)
{
	v2p_t v2p;
	v2p.position.x = RangeMap(input.localPosition.x, OrthoMinX, OrthoMaxX, -1, 1);
	v2p.position.y = RangeMap(input.localPosition.y, OrthoMinY, OrthoMaxY, -1, 1);
	v2p.position.z = RangeMap(input.localPosition.z, OrthoMinZ, OrthoMaxZ, 0, 1);
	v2p.position.w = 1;

	v2p.color = input.color;
	v2p.uv = input.uv;
	return v2p;
}
