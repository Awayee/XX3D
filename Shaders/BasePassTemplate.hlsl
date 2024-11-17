#include "GBufferCommon.hlsli"

//vs
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
    [[vk::location(1)]] float3 inNormal : NORMAL0;
    [[vk::location(2)]] float3 inTangent: TANGENT0;
    [[vk::location(3)]] float2 inUV: TEXCOORD0;
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
struct InstanceData {
#if defined(INSTANCE_HALF_FLOAT)
    half4x4 TransformMat;
#else
	float4x4 TransformMat;
#endif
};

float3x3 GetNormalTransform(float4x4 t)
{
    return float3x3(
		t[0][0], t[0][1], t[0][2],
		t[1][0], t[1][1], t[1][2],
		t[2][0], t[2][1], t[2][2]
    );
}

[[vk::binding(0, 1)]] StructuredBuffer<InstanceData> uInstanceData;
[[vk::binding(1, 1)]] StructuredBuffer<uint> uInstanceID;

#define CoordTransform (uInstanceData[uInstanceID[instanceID]].TransformMat)
#define NormalTransform (GetNormalTransform(uInstanceData[uInstanceID[instanceID]].TransformMat))
#else
struct ModelUBO {
    float4x4 ModelMat;
    float4x4 ModelInvMat;
};
[[vk::binding(0, 1)]] cbuffer uModel { ModelUBO uModel; };
#define CoordTransform uModel.ModelMat
#define NormalTransform transpose((float3x3)uModel.ModelInvMat)
#endif

VSOutput MainVS(VSInput vIn, uint vertexID: SV_VERTEXID, uint instanceID: SV_InstanceID) {
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