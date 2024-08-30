// vs
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
};

struct VSOutput {
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float3 UV : TEXCOORD0;
};

struct CameraUBO{
    float4x4 View;
    float4x4 Proj;
    float4x4 VP;
    float4x4 InvVP;
    float4 Pos;
};

[[vk::binding(0, 0)]]cbuffer uCamera{ CameraUBO uCamera; };

VSOutput MainVS(VSInput vIn) {
    VSOutput vOut = (VSOutput) 0;
    vOut.UV = vIn.inPosition;
    float3x3 skyView = (float3x3) uCamera.View;
    float4 clipPos = mul(uCamera.Proj, float4(mul(skyView, vIn.inPosition), 1.0));
    vOut.Position = clipPos.xyww;
    return vOut;
}

// ps
struct PSOutput{
    half4 OutColor : SV_TARGET;
};

[[vk::binding(1,0)]] TextureCube<float4> inSkyBoxMap;
[[vk::binding(2,0)]] SamplerState inSampler;

PSOutput MainPS(VSOutput pIn) {
    PSOutput pOut = (PSOutput) 0;
    float3 color = inSkyBoxMap.Sample(inSampler, pIn.UV).rgb;
    pOut.OutColor = float4(color, 1.0);
    return pOut;
}