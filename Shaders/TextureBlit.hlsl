#define THREAD_ROW_NUM 16

[[vk::binding(0, 0)]] Texture2D<float> srcTex;
[[vk::binding(1, 0)]] RWTexture2D<float> dstTex;
[[vk::binding(2, 0)]] cbuffer uSizeInfo{float4 uSizeInfo;}
[[vk::binding(3, 0)]] sampler inSampler;

[numthreads(THREAD_ROW_NUM, THREAD_ROW_NUM, 1)]
void MainCS(uint3 tid : SV_DispatchThreadID){
    // output 2x2 pixels per thread.
    int2 outputCoordStart = tid.xy * 2;
    float2 outputSize = uSizeInfo.xy;

    int2 outputCoord = outputCoordStart;
    float2 outputUV = (float2) outputCoord / outputSize;
    dstTex[outputCoord] = srcTex.SampleLevel(inSampler, outputUV, 0);

    outputCoord = outputCoordStart + int2(1, 0);
    outputUV = (float2) outputCoord / outputSize;
    dstTex[outputCoord] = srcTex.SampleLevel(inSampler, outputUV, 0);

    outputCoord = outputCoordStart + int2(0, 1);
    outputUV = (float2) outputCoord / outputSize;
    dstTex[outputCoord] = srcTex.SampleLevel(inSampler, outputUV, 0);

    outputCoord = outputCoordStart + int2(1, 1);
    outputUV = (float2) outputCoord / outputSize;
    dstTex[outputCoord] = srcTex.SampleLevel(inSampler, outputUV, 0);
}