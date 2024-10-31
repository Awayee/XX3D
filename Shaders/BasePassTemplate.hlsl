#include "GBufferCommon.hlsli"

//vs
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
    [[vk::location(1)]] float3 inNormal : NORMAL0;
    [[vk::location(2)]] float3 inTangent: TANGENT0;
    [[vk::location(3)]] float2 inUV: TEXCOORD0;
#if defined(INSTANCED)
    [[vk::location(4)]] half4 t0 : INSTANCE_TRANSFORM0;
    [[vk::location(5)]] half4 t1 : INSTANCE_TRANSFORM1;
    [[vk::location(6)]] half4 t2 : INSTANCE_TRANSFORM2;
    [[vk::location(7)]] half4 t3 : INSTANCE_TRANSFORM3;

	half4x4 GetInstanceTransform() {
        return half4x4(
        t0.x, t1.x, t2.x, t3.x,
        t0.y, t1.y, t2.y, t3.y,
        t0.z, t1.z, t2.z, t3.z,
        t0.w, t1.w, t2.w, t3.w);
    }
    half3x3 GetInstanceNormalTransform() {
        return half3x3(
        t0.x, t1.x, t2.x,
        t0.y, t1.y, t2.y,
        t0.z, t1.z, t2.z);
    }
#endif
};

struct VSOutput{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float3 WorldNormal : NORMAL;
    [[vk::location(1)]] float2 UV : TEXCOORD0;
};

struct CameraUBO{
    float4x4 View;
    float4x4 Proj;
    float4x4 VP;
    float4x4 InvVP;
    float4 Pos;
};
[[vk::binding(0, 0)]] cbuffer uCamera { CameraUBO uCamera; };

#if defined(INSTANCED)
#define CoordTransform (vIn.GetInstanceTransform())
#define NormalTransform ((float3x3)vIn.GetInstanceTransform())
#else
struct ModelUBO {
    float4x4 ModelMat;
    float4x4 ModelInvMat;
};
[[vk::binding(0, 1)]] cbuffer uModel { ModelUBO uModel; };
#define CoordTransform uModel.ModelMat
#define NormalTransform transpose((float3x3)uModel.ModelInvMat)
#endif

VSOutput MainVS(VSInput vIn, uint vertexID: SV_VERTEXID) {
    VSOutput output;
    output.Position = mul(uCamera.VP, mul(CoordTransform, float4(vIn.inPosition.xyz, 1.0)));
    output.WorldNormal = mul(NormalTransform, vIn.inNormal);
    output.UV = vIn.inUV;
    return output;
}

// bindings
%s

struct PSOutput {
    float4 GBuffer0: SV_TARGET0;
    float4 GBuffer1: SV_TARGET1;
};

PSOutput MainPS(VSOutput pIn) {
    float3 worldNormal01 = pIn.WorldNormal * 0.5 + 0.5;
    GBufferData data = (GBufferData)0;
    data.Normal = worldNormal01;
    // GBuffer output
    {%s}
    PSOutput psOut = (PSOutput)0;
    EncodeGBuffer(data, psOut.GBuffer0, psOut.GBuffer1);
    return psOut;
}