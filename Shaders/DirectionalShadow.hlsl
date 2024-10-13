//vs
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
#if defined(INSTANCED)
    [[vk::location(1)]] float4 t0 : INSTANCE_TRANSFORM0;
    [[vk::location(2)]] float4 t1 : INSTANCE_TRANSFORM1;
    [[vk::location(3)]] float4 t2 : INSTANCE_TRANSFORM2;
    [[vk::location(4)]] float4 t3 : INSTANCE_TRANSFORM3;

    float4x4 GetInstanceTransform() {
        return float4x4(
        t0.x, t1.x, t2.x, t3.x,
        t0.y, t1.y, t2.y, t3.y,
        t0.z, t1.z, t2.z, t3.z,
        t0.w, t1.w, t2.w, t3.w);
    }
#endif
};

struct VSOutput{
    float4 Position : SV_POSITION;
};

struct ShadowCameraUBO {
    float4x4 VP;
};
[[vk::binding(0, 0)]] cbuffer uShadowCamera { ShadowCameraUBO uShadowCamera; };

#if defined(INSTANCED)
#define CoordTransform (vIn.GetInstanceTransform())
#else
struct ModelUBO {
    float4x4 ModelMat;
    float4x4 ModelInvMat;
};
[[vk::binding(0, 1)]] cbuffer uModel { ModelUBO uModel; };
#define CoordTransform uModel.ModelMat
#endif

VSOutput MainVS(VSInput vIn, uint vertexID: SV_VERTEXID) {
    VSOutput output;
    output.Position = mul(uShadowCamera.VP, mul(CoordTransform, float4(vIn.inPosition.xyz, 1.0)));
    return output;
}

// error in vulkan if there is no PS.
struct PSOutput
{
};

PSOutput MainPS(VSOutput pIn) {
    return (PSOutput) 0;
}