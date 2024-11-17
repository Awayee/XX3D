
float FurtherDepth(float a, float b)
{
    return max(a, b);
}

float CloserDepth(float a, float b)
{
    return min(a, b);
}

float4 FurtherDepth(float4 a, float4 b)
{
    return max(a, b);
}

float4 CloserDepth(float4 a, float4 b)
{
    return min(a, b);
}

// return a closer than b
float IsDepthCloser(float a, float b)
{
    return step(a, b);
}

// return a further than b
float IsDepthFurther(float a, float b)
{
    return step(b, a);
}

// return a further than b
bool IsDepthFurtherOrEqual(float a, float b) {
    return a >= b;
}

// return a closer than b
bool IsDepthCloserOrEqual(float a, float b) {
    return a <= b;
}