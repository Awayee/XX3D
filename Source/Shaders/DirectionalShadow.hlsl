//vs
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

struct ShadowCameraUBO {
    float4x4 VP;
};

struct ModelUBO {
    float4x4 ModelMat;
    float4x4 ModelInvMat;
};

[[vk::binding(0, 0)]] cbuffer uShadowCamera { ShadowCameraUBO uShadowCamera; };
[[vk::binding(0, 1)]] cbuffer uModel { ModelUBO uModel; };

VSOutput MainVS(VSInput vIn, uint vertexID: SV_VERTEXID) {
    VSOutput output;
    output.Position = mul(uShadowCamera.VP, mul(uModel.ModelMat, float4(vIn.inPosition.xyz, 1.0)));
    return output;
}

// error in vulkan if there is no PS.
struct PSOutput
{
};

PSOutput MainPS(VSOutput pIn) {
    return (PSOutput) 0;
}