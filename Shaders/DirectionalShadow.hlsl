//vs
//#define INSTANCE_HALF_FLOAT
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
};

struct VSOutput{
    float4 Position : SV_POSITION;
};

struct ShadowCameraUBO {
    float4x4 VP;
};
[[vk::binding(0, 0)]] cbuffer uShadowCamera { ShadowCameraUBO uShadowCamera; };

#if defined(INSTANCED)
struct InstanceData {
#ifdef INSTANCE_HALF_FLOAT
    half4x4 TransformMat;
#else
	float4x4 TransformMat;
#endif
};

[[vk::binding(0, 1)]] StructuredBuffer<InstanceData> uInstanceData;
[[vk::binding(1, 1)]] StructuredBuffer<uint> uInstanceID;

#define CoordTransform (uInstanceData[uInstanceID[instanceID]].TransformMat)

#else
struct ModelUBO {
    float4x4 ModelMat;
    float4x4 ModelInvMat;
};
[[vk::binding(0, 1)]] cbuffer uModel { ModelUBO uModel; };
#define CoordTransform uModel.ModelMat
#endif

VSOutput MainVS(VSInput vIn, uint vertexID: SV_VERTEXID, uint instanceID: SV_InstanceID) {
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