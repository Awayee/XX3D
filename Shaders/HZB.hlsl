#define THREAD_ROW_NUM 16
#include "Common.hlsli"

[[vk::binding(0, 0)]] Texture2D<float> inDepth;
[[vk::binding(1, 0)]] cbuffer uSizeInfo{int4 uSizeInfo;}
#if defined(FURTHEST)
[[vk::binding(2, 0)]] RWTexture2D<float> outFurthest;
#ifdef CLOSEST
[[vk::binding(3, 0)]] RWTexture2D<float> outCloset;
#endif
#elif defined(CLOSEST)
[[vk::binding(2, 0)]] RWTexture2D<float> outCloset;
#endif

// outputs 2x2 pixels per thread, outputUV is the start pixel.
void Output2x2(int2 outputUV) {
    float4 inVal[4];
    // (0, 0)
    int2 inUV = outputUV * 2;
    inVal[0].x = inDepth.Load(int3(inUV, 0)).x;
    inVal[1].x = inDepth.Load(int3(inUV + int2(0, 1), 0)).x;
    inVal[2].x = inDepth.Load(int3(inUV + int2(1, 0), 0)).x;
    inVal[3].x = inDepth.Load(int3(inUV + int2(1, 1), 0)).x;

    // (1, 0)
    inUV = (outputUV + int2(1, 0)) * 2;
    inVal[0].y = inDepth.Load(int3(inUV, 0)).x;
    inVal[1].y = inDepth.Load(int3(inUV + int2(0, 1), 0)).x;
    inVal[2].y = inDepth.Load(int3(inUV + int2(1, 0), 0)).x;
    inVal[3].y = inDepth.Load(int3(inUV + int2(1, 1), 0)).x;

    // (0, 1)
    inUV = (outputUV + int2(0, 1)) * 2;
    inVal[0].z = inDepth.Load(int3(inUV, 0)).x;
    inVal[1].z = inDepth.Load(int3(inUV + int2(0, 1), 0)).x;
    inVal[2].z = inDepth.Load(int3(inUV + int2(1, 0), 0)).x;
    inVal[3].z = inDepth.Load(int3(inUV + int2(1, 1), 0)).x;

    // (1, 1)
    inUV = (outputUV + int2(1, 1)) * 2;
    inVal[0].w = inDepth.Load(int3(inUV, 0)).x;
    inVal[1].w = inDepth.Load(int3(inUV + int2(0, 1), 0)).x;
    inVal[2].w = inDepth.Load(int3(inUV + int2(1, 0), 0)).x;
    inVal[3].w = inDepth.Load(int3(inUV + int2(1, 1), 0)).x;

#if defined(FURTHEST)
    float4 further0 = FurtherDepth(inVal[0], inVal[1]);
    float4 further1 = FurtherDepth(inVal[2], inVal[3]);
    float4 furthest = FurtherDepth(further0, further1);
    outFurthest[outputUV] = furthest.x;
    outFurthest[outputUV + int2(1, 0)] = furthest.y;
    outFurthest[outputUV + int2(0, 1)] = furthest.z;
    outFurthest[outputUV + int2(1, 1)] = furthest.w;
#endif
#if defined(CLOSEST)
    float4 closer0 = CloserDepth(inVal[0], inVal[1]);
    float4 closer1 = CloserDepth(inVal[2], inVal[3]);
    float4 closest = CloserDepth(closer0, closer1);
    outFurthest[outputUV] = closest.x;
    outFurthest[outputUV + int2(1, 0)] = closest.y;
    outFurthest[outputUV + int2(0, 1)] = closest.z;
    outFurthest[outputUV + int2(1, 1)] = closest.w;
#endif
}

// outputs just 1 pixel
void Output1x1(int2 outputUV) {
    int2 inUV = min(outputUV * 2, uSizeInfo.xy);
    float inVal[4];
    inVal[0] = inDepth.Load(int3(inUV, 0)).x;
    inVal[1] = inDepth.Load(int3(inUV + int2(0, 1), 0)).x;
    inVal[2] = inDepth.Load(int3(inUV + int2(1, 0), 0)).x;
    inVal[3] = inDepth.Load(int3(inUV + int2(1, 1), 0)).x;
#if defined(FURTHEST)
    float further0 = FurtherDepth(inVal[0], inVal[1]);
    float further1 = FurtherDepth(inVal[2], inVal[3]);
    float furthest = FurtherDepth(further0, further1);
    outFurthest[outputUV] = furthest;
#endif
#if defined(CLOSEST)
    float closer0 = CloserDepth(inVal[0], inVal[1]);
    float closer1 = CloserDepth(inVal[2], inVal[3]);
    float closest = CloserDepth(closer0, closer1);
    outFurthest[outputUV] = closest;
#endif
}

[numthreads(THREAD_ROW_NUM, THREAD_ROW_NUM, 1)]
void MainCS(uint3 tid : SV_DispatchThreadID)
{
#if defined(OUTPUT2X2)
    int2 outputUVStart = tid.xy * 2;
    Output2x2(outputUVStart);
#else
    int2 outputUV = tid.xy;
    Output1x1(outputUV);
#endif
}