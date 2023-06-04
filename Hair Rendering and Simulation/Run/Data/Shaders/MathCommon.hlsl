#define PI  3.14159265358979323846  
#define HAIR_REFRACTIVE_INDEX 1.55
#define EPSILON 0.001

float3 ProjectVecOntoPlane(float3 disp, float3 planeNormal)
{
    return disp - dot(disp, planeNormal) * planeNormal; // Removing the plane normal parts. Similar to when reflecting a vector
}


float SafeAcos(float cosValue)
{
    cosValue = clamp(cosValue, -1.0f + EPSILON, 1.0f - EPSILON);
    
    
    return acos(cosValue);
}

float GetAngleBetVectors(float3 vecA, float3 vecB)
{
    return SafeAcos(dot(vecA, vecB));
}

float GetAngleBetDirAndPlane(float3 dir, float3 normal)
{
    float angle = SafeAcos(dot(dir, normal));
    
    static const float halfPi = 0.5f * PI; // 90 degrees
    
    return halfPi - angle;
}

float GetAngleBetVectorsAroundPlane(const float3 vecA, const float3 vecB, const float3 planeNormal) // this is because some angles might be negative (with the plane normal pointing back) so they need to be transformed
{
    float3 normal = normalize(cross(vecB, vecA));
    
    float3 disp = planeNormal - normal;
    float dotProd = dot(vecA, vecB);
    float angle = SafeAcos(dotProd);
    
    /*
        EPSILON SQR IS TOO SMALL! SOME CORRECT ANGLES WHERE FAILING THE TEST
        CAUSING BLACK PIXELS IN BETWEEN LIT PIXELS.
    */
    if (length(disp) < EPSILON /** EPSILON*/)
    {
        return angle;
    }
    else
    {
        static const float twoPi = 2.0f * PI;
        return (twoPi) - angle;

    }
    
}

float GetFresnel(float refrIndexPrime, float refrIndex, float x) // Shclick's approximation for Fresnel. X is typically the dot product of the view and half view vectors or cos theta Proven Correct
{
    
    float diffRefSqr = (refrIndexPrime - refrIndex) * (refrIndexPrime - refrIndex);
    
    float sumRefSqr = (refrIndexPrime + refrIndex) * (refrIndexPrime + refrIndex);

    float initialTerm = diffRefSqr / sumRefSqr;
    
    float oneMinusX = (1.0f - x);
    
    return initialTerm + (1.0f - initialTerm) * pow(oneMinusX, 5.0f);

}


float ConvertToRadians(float angleDegrees)
{
    static float radiansConv = PI / 180.0f;
    
    return angleDegrees * radiansConv;
}

float ComputeLongitudinalScattering(float scaleShift, float roughness, float angleToLight, float angleToView)  // alpha, beta, thetaI thetaR  Proven Correct
{
    static const float sqrRoot2PI = sqrt(2.0f * PI);
    
    float firstTerm = 1.0f / (roughness * sqrRoot2PI);
    
    float exponentTop = sin(angleToLight) + sin(angleToView) - scaleShift;
    float exponentTopSqr = exponentTop * exponentTop;
    
    float exponent = (-exponentTopSqr) / (2.0f * roughness * roughness);
    float expResult = exp(exponent);
    
    return firstTerm * expResult;
}

float3 ComputeAzimuthalR(float3 lightDir, float3 viewDir, float deltaTheta, float phi) // lightBounce = 0  Proven Correct
{
    float sqrRootTerm = 0.5f + (0.5f * dot(lightDir, viewDir)); // Preferring sqrt over cos
    float absorptionX = sqrt(sqrRootTerm);
    float absorption = GetFresnel(1.0f, HAIR_REFRACTIVE_INDEX, absorptionX);
    
    
    float azimuthal = 0.25f * sqrt(0.5f + (0.5f * cos(phi)));
    float finalAzimuthal = azimuthal * absorption;
    
    return float3(finalAzimuthal, finalAzimuthal, finalAzimuthal);
}


float3 ComputeAzimuthalTT(float3 lightDir, float3 viewDir, float deltaTheta, float phi, float4 modelColor) // lightBounce = 1
{
    float cosThetaD = cos(deltaTheta);
    float cosPhi = cos(phi);
    float cosHalfPhi = cos(phi * 0.5f);
    
    float refractionPrime = (1.19f / cosThetaD) + (0.36f * cosThetaD);
    float a = 1.0f / refractionPrime;
    float h = (1.0f + a * (0.6f - (0.8f * cosPhi))) * cosHalfPhi; // Height till the incident ray of light
    
    float hSqr = h * h;
    
    float fresnelX = cosThetaD * sqrt(1.0f - hSqr);
    float fresnel = GetFresnel(refractionPrime, HAIR_REFRACTIVE_INDEX, fresnelX);
    
    float expTop = sqrt(1.0f - (hSqr * a * a));
    float expBottom = 2.0f * cosThetaD;
    
    float transmExp = expTop / expBottom;
    
    float colorPickR = pow(modelColor.r, transmExp);
    float colorPickG = pow(modelColor.g, transmExp);
    float colorPickB = pow(modelColor.b, transmExp);
    
    float3 transmission = float3(colorPickR, colorPickG, colorPickB);
    
    float oneMinusFSqr = (1.0f - fresnel) * (1.0f - fresnel);
    float3 absorption = oneMinusFSqr * transmission;
    float otherCosPhi = cos(phi - 3.98f);
    float distribution = exp((-3.65 * cosPhi) - 3.98f);
 
    
    return (absorption * distribution); // test how it looks without 0.25f 
}

float3 ComputeAzimuthalTRT(float3 lightDir, float3 viewDir, float deltaTheta, float phi, float4 modelColor) // lightBounce = 2
{
    
    float cosThetaD = cos(deltaTheta);
    float cosPhi = cos(phi);
    float cosHalfPhi = cos(phi * 0.5f);
    float refractionPrime = (1.19f / cosThetaD) + (0.36f * cosThetaD);
    
    
    float h = 0.5f * sqrt(3.0f);
    float hSqr = h * h;
    
    float fresnelX = cosThetaD * sqrt(1.0f - hSqr);
    float fresnel = GetFresnel(1.0f, HAIR_REFRACTIVE_INDEX, fresnelX);
    
    float transmExp = 0.8f / cosThetaD;
    float distribution = exp((17.0f * cosPhi) - 16.78);
    
    float colorPickR = pow(modelColor.r, transmExp);
    float colorPickG = pow(modelColor.g, transmExp);
    float colorPickB = pow(modelColor.b, transmExp);
    
    float3 transmission = float3(colorPickR, colorPickG, colorPickB);
    float oneMinusFSqr = (1.0f - fresnel) * (1.0f - fresnel);
    float3 absorption = oneMinusFSqr * fresnel * transmission * transmission;
    
    return (/*0.25f **/ absorption * distribution); // test how it looks without 0.25f 
}


float3 ComputeAzimuthal(int lightBounce, float3 lightDir, float3 viewDir, float deltaTheta, float phi, float4 modelColor)  // The 2 angles needed for lighting models from the hair cross section
{
    
    switch (lightBounce)
    {
        case 0:
            return ComputeAzimuthalR(lightDir, viewDir, deltaTheta, phi);
            break;
        
        case 1:
            return ComputeAzimuthalTT(lightDir, viewDir, deltaTheta, phi, modelColor);
            break;
        
        case 2:
            return ComputeAzimuthalTRT(lightDir, viewDir, deltaTheta, phi, modelColor);
            break;
    }
    
    return float3(1.0f, 1.0f, 0.0f);
}


void GetOrthonormalBasisFrisvad(float3 iBasis, out float3 jBasis, out float3 kBasis) // There are multiple ways of finding an orthonormal basis. This has been studied to be fast as it does not need square roots. Frisvad method
{
    if (iBasis.z < -0.9999999f) // Handle the singularity
    {
        jBasis = float3(0.0f, -1.0f, 0.0f);
        kBasis = float3(-1.0f, 0.0f, 0.0f);
        return;
    }
    const float a = 1.0f / (1.0f + iBasis.z);
    const float b = -iBasis.x * iBasis.y * a;
    jBasis = float3(1.0f - iBasis.x * iBasis.x * a, b, -iBasis.x);
    kBasis = float3(b, 1.0f - iBasis.y * iBasis.y * a, -iBasis.y);
}

float3 RotateVectorAroundAxis(float3 dir, float rotationDeg, float3 axis)
{
    float3 rotatedVec = dir * cos(rotationDeg) + (cross(axis, dir)) * sin(rotationDeg) + axis * (dot(axis, dir)) * (1 - cos(rotationDeg)); // Rodrigues' rotation formula
    
    rotatedVec = normalize(rotatedVec);
    
    return rotatedVec;
    
}

float CustomHash(float n)
{
    return frac(sin(n) * 43758.5453);
}

float CustomNoise(float3 x)
{
    // The noise function returns a value in the range -1.0f -> 1.0f

    float3 p = floor(x);
    float3 f = frac(x);

    f = f * f * (3.0 - 2.0 * f);
    float n = p.x + p.y * 57.0 + 113.0 * p.z;

    return lerp(lerp(lerp(CustomHash(n + 0.0), CustomHash(n + 1.0), f.x),
                   lerp(CustomHash(n + 57.0), CustomHash(n + 58.0), f.x), f.y),
               lerp(lerp(CustomHash(n + 113.0), CustomHash(n + 114.0), f.x),
                   lerp(CustomHash(n + 170.0), CustomHash(n + 171.0), f.x), f.y), f.z);
}


float SDFSphere(float3 position, float3 sphereCenter, float radius)
{
    float3 distToPosition = position - sphereCenter;
    
    return length(distToPosition) - radius;
}

float3 PushPointOutOfSphere(float3 position, float3 sphereCenter, float radius)
{
    float overlapDist = SDFSphere(position, sphereCenter, radius);
    if (overlapDist < 0.0f)
    {
        float3 pushDirection = normalize(position - sphereCenter);
   
        float3 endPosition = position + (pushDirection * -overlapDist);
        return endPosition;
    }
    
    return position;
    
}

int3 GetCoordsForPosition(float3 position, float3 gridOrigin)
{
    float3 dispToPosition = position - gridOrigin;

    int3 roundedDown = int3(floor(dispToPosition));
    
    return roundedDown;
}

int GetIndexForCoords(int3 coords, int3 dimensions)
{
    int index = coords.x + (coords.y * dimensions.x) + (coords.z * dimensions.x * dimensions.y);
    
    return index;
}

float3 EvaluateBezierCurve(float t, float3 a, float3 b, float3 c, float3 d)
{
    float3 t3D = float3(t, t, t);
    float3 ab = lerp(a, b, t3D);
    float3 bc = lerp(b, c, t3D);
    float3 cd = lerp(c, d, t3D);
    
    float3 abc = lerp(ab, bc, t3D);
    float3 bcd = lerp(bc, cd, t3D);
    
    float3 finalPosition = lerp(abc, bcd, t3D);
    
    return finalPosition;
}

float1 GetFractionWithin(float1 inValue, float1 inStart, float1 inEnd)
{
    if (inStart == inEnd)
        return 0.5f;
    float1 range = inEnd - inStart;
    return (inValue - inStart) / range;
}
float1 RangeMap(float1 inValue, float1 inStart, float1 inEnd, float1 outStart, float1 outEnd)
{
    float1 fraction = GetFractionWithin(inValue, inStart, inEnd);
    return lerp(outStart, outEnd, fraction);
}

float ComputeDiffuseLighting(float3 lightPosition, float3 objectPosition, float3 tangent, float diffuseCoef, /*bool useAcos,*/ bool invertLightDir)
{
    float3 vecToLight = lightPosition - objectPosition;
    
    vecToLight = normalize(vecToLight);
    
    [flatten]
    if (invertLightDir)
    {
        vecToLight *= -1.0f;
    }
    
    float diffuse = 0.0f;
    diffuse = sqrt(1.0f - (dot(tangent, vecToLight) * dot(tangent, vecToLight)));
    //diffuse = sin(dot(tangent, vecToLight));
    
    diffuse = saturate(diffuseCoef * diffuse);
    
    return diffuse;
}

float ComputeSpecularLighting(float3 lightPosition, float3 objectPosition, float3 eyePosition, float3 tangent, float specularExp, float specularCoeff)
{
    tangent = normalize(tangent);
    float3 vecToLight = lightPosition - objectPosition;
    vecToLight = normalize(vecToLight);
 
    float3 vecToEye = eyePosition - objectPosition;
    vecToEye = normalize(vecToEye);
    
    
    float dotProducts = dot(tangent, vecToLight) * dot(tangent, vecToEye);
    //float crossProducts = length(cross(tangent, vecToLight)) * length(cross(tangent, vecToEye));
    float dotTangentLight = dot(tangent, vecToLight);
    float angleTangentLight = SafeAcos(dotTangentLight);
    
    float dotTangentEye = dot(tangent, vecToEye);
    float angleTangentEye = SafeAcos(dotTangentEye);
    float crossProducts = sin(angleTangentLight) * sin(angleTangentEye);
    //crossProducts = saturate(crossProducts);
    float exponent = pow(2, float(specularExp));
    float sum = (dotProducts + crossProducts);
    sum = saturate(sum);
    float specular = specularCoeff * pow(sum, exponent); // POW RETURNS NAN ON NEGATIVES
    
    return specular;

}