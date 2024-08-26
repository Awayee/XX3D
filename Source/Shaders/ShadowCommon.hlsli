#define CASCADE_NUM 4
#define SHADOW_VAL 0.1


float2 GetTexelSize2D(Texture2DArray<float> tex){
    uint w, h, l, m;
    tex.GetDimensions(0, w, h, l, m);
    return float2(1.0 / (float) w, 1.0 / (float) h);
}


#define PCF_NUM 25
float2 PoissonDisk25(uint num){
    const float2 PoissonDistVal[PCF_NUM] ={
        float2(-0.978698, -0.0884121),
	    float2(-0.841121, 0.521165),
	    float2(-0.71746, -0.50322),
	    float2(-0.702933, 0.903134),
	    float2(-0.663198, 0.15482),
	    float2(-0.495102, -0.232887),
	    float2(-0.364238, -0.961791),
	    float2(-0.345866, -0.564379),
	    float2(-0.325663, 0.64037),
	    float2(-0.182714, 0.321329),
	    float2(-0.142613, -0.0227363),
	    float2(-0.0564287, -0.36729),
	    float2(-0.0185858, 0.918882),
	    float2(0.0381787, -0.728996),
	    float2(0.16599, 0.093112),
	    float2(0.253639, 0.719535),
	    float2(0.369549, -0.655019),
	    float2(0.423627, 0.429975),
	    float2(0.530747, -0.364971),
	    float2(0.566027, -0.940489),
	    float2(0.639332, 0.0284127),
	    float2(0.652089, 0.669668),
	    float2(0.773797, 0.345012),
	    float2(0.968871, 0.840449),
	    float2(0.991882, -0.657338)
    };
    return PoissonDistVal[num];
}

//float PoissonDisk16(uint num) {
//    const float2 Values[16] ={
//        float2(-0.94201624, -0.39906216),
//		float2(0.94558609, -0.76890725),
//	    float2(-0.094184101, -0.92938870),
//	    float2(0.34495938, 0.29387760),
//	    float2(-0.91588581, 0.45771432),
//	    float2(-0.81544232, -0.87912464),
//	    float2(-0.38277543, 0.27676845),
//	    float2(0.97484398, 0.75648379),
//	    float2(0.44323325, -0.97511554),
//	    float2(0.53742981, -0.47373420),
//	    float2(-0.26496911, -0.41893023),
//	    float2(0.79197514, 0.19090188),
//	    float2(-0.24188840, 0.99706507),
//	    float2(-0.81409955, 0.91437590),
//	    float2(0.19984126, 0.78641367),
//	    float2(0.14383161, -0.14100790)
//    };
//    return Values[num];
//}

// return 1.0 if a is closer than b else 0.0
float IsDepthCloser(float a, float b){
    return step(a, b);
}

float CalcShadowVal(Texture2DArray<float> inShadowMap, SamplerState inSampler, uint cascade, float2 uv, float z){
    float closestZ = inShadowMap.Sample(inSampler, float3(uv, (float) cascade)).r;
    return (1.0 + (SHADOW_VAL - 1.0) * IsDepthCloser(closestZ, z));
}

float PCF25(Texture2DArray<float> inShadowMap, SamplerState inSampler, uint cascade, float3 shadowCoords, float radius){
    float2 texelSize = GetTexelSize2D(inShadowMap);
    float shadowVal = 0.0f;
	[unroll]
    for (uint i = 0; i < PCF_NUM; ++i)
    {
        float2 offsetUV = shadowCoords.xy + texelSize * radius * PoissonDisk25(i);
        shadowVal += CalcShadowVal(inShadowMap, inSampler, cascade, offsetUV, shadowCoords.z); // in shadow distance and current z is greater than closest z.
    }
    return shadowVal / PCF_NUM;
}

float PCFByMicrosoft(Texture2DArray<float> inShadowMap, SamplerState inSampler, uint cascade, float3 shadowCoords, float radius){
    float2 texelSize = GetTexelSize2D(inShadowMap);
    float d1 = radius * texelSize.x * 0.125;
    float d2 = radius * texelSize.x * 0.875;
    float d3 = radius * texelSize.x * 0.625;
    float d4 = radius * texelSize.x * 0.375;
    float2 uv = shadowCoords.xy;
    float result = (
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(-d2, d1), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(-d1, -d2), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(d2, -d1), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(d1, d2), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(-d4, d3), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(-d3, -d4), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(d4, -d3), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv + float2(d4, d3), shadowCoords.z) +
        CalcShadowVal(inShadowMap, inSampler, cascade, uv, shadowCoords.z) * 2.0f) / 10.0f;
    return result;
}

// test whether components in range (0, 1)
float3 TestRange01(float3 projectCoords){
    return float3(
		step(0, projectCoords.x) * step(projectCoords.x, 1),
		step(0, projectCoords.y) * step(projectCoords.y, 1),
		step(0, projectCoords.z) * step(projectCoords.z, 1));
}

float FindBlocker(Texture2DArray<float> inShadowMap, SamplerState inSampler, uint cascade, float3 shadowCoords){
    const float lightRadius = 1.0f;
    float2 lightRadius2 = float2(lightRadius, lightRadius);
    const float2 texelSize = GetTexelSize2D(inShadowMap);
    float2 depthAndCount = float2(0.0, 0.0);
    for (uint ns = 0; ns < PCF_NUM; ++ns){
        float2 sampleCoord = (lightRadius2 * PoissonDisk25(ns)) * texelSize + shadowCoords.xy;
        float cloestZ = inShadowMap.Sample(inSampler, float3(sampleCoord, (float) cascade));
        if (IsDepthCloser(cloestZ, shadowCoords.z) > 0.0){
            depthAndCount += float2(cloestZ, 1.0);
        }
    }
    if (depthAndCount.y > 0.0f){
        return depthAndCount.x / depthAndCount.y;
    }
    return 1.0;
}

float PCSS(Texture2DArray<float> inShadowMap, SamplerState inSampler, uint cascade, float3 shadowCoords){
	// STEP 1: avgblocker depth
    float avgBlockerDepth = FindBlocker(inShadowMap, inSampler, cascade, shadowCoords);
	  // STEP 2: penumbra size
    const float lightWidth = 2.0f;
    float penumbraSize = max(shadowCoords.z - avgBlockerDepth, 0.0) / avgBlockerDepth * lightWidth;
    return PCF25(inShadowMap, inSampler, cascade, shadowCoords, penumbraSize + 1.0);
}