// Max 8 kernels out
#define PI 3.14159265359

float GetGaussian1D(float sigma, float dist)
{
    float sigmaSqr = sigma * sigma;
    float distSqr = dist * dist;
    float sigmaSqrTwo = 2.0f * sigmaSqr;
    
    float exponent = -(distSqr / (sigmaSqrTwo));
    float eCoeff = exp(exponent);
    
    float scalarCoef = 1.0f / (sqrt(sigmaSqrTwo * PI));
    
    return scalarCoef * eCoeff;
}

// This would be worth hardcoding later
void GetGaussianKernels(unsigned int kernelSize, float sigma,  out float kernels[8])
{
    float total = 0.0f;
    unsigned int totalTermsCount = (kernelSize + 1);
    unsigned int halfKernelSize = kernelSize / 2;
    for (unsigned int termInd = 0; termInd < kernelSize; termInd++)
    {
        float gaussianSample = GetGaussian1D(sigma, termInd);
        total += gaussianSample;
        if(termInd <= halfKernelSize)
        {
            kernels[termInd] = gaussianSample;
        }
    }
    
    // Make the array symetrical
    for (unsigned int simmetryInd = 0; simmetryInd < (kernelSize - halfKernelSize); simmetryInd++)
    {
        kernels[simmetryInd + halfKernelSize] = kernels[halfKernelSize - simmetryInd];
    }
    
    float invTotal = 1.0f / total;
    for (unsigned int normalInd = 0; normalInd < kernelSize; normalInd++)
    {
        kernels[normalInd] /= invTotal;
    }
}