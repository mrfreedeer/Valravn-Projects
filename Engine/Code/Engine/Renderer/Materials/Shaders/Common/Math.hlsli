
template<typename t_type>
t_type GetFractionWithin(t_type inValue, t_type inStart, t_type inEnd)
{
    //if (inStart == inEnd)
    //    return 0.5f;
    t_type range = inEnd - inStart;
    return (inValue - inStart) / range;
}

//float4 GetFractionWithin(float4 inValue, float4 inStart, float4 inEnd)
//{
//    float4 range = inEnd - inStart;
//    return (inValue - inStart) / range;
//}

float1 Interpolate(float1 outStart, float1 outEnd, float1 fraction)
{
    return outStart + fraction * (outEnd - outStart);
}
float4 Interpolate(float4 outStart, float4 outEnd, float4 fraction)
{
    return outStart + fraction * (outEnd - outStart);
}

float1 RangeMap(float1 inValue, float1 inStart, float1 inEnd, float1 outStart, float1 outEnd)
{
    float1 fraction = GetFractionWithin(inValue, inStart, inEnd);
    return Interpolate(outStart, outEnd, fraction);
}




float4 RangeMap(float4 inValue, float4 inStart, float4 inEnd, float4 outStart, float4 outEnd)
{
    float4 fraction = GetFractionWithin(inValue, inStart, inEnd);
    return Interpolate(outStart, outEnd, fraction);
}

