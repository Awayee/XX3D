//to avoid more macros, the common struct used by ps, vs and other stage is defined firstly
struct VSOutput{
    float4 Position: SV_POSITION;
    [[vk::location(0)]] float3 worldNormal: NORMAL;
    [[vk::location(1)]] float2 UV: TEXCOORD0;
};

//vs
struct VSInput {
    [[vk::location(0)]] float3 inPosition : POSITION0;
    [[vk::location(1)]] float3 inNormal : NORMAL0;
    [[vk::location(2)]] float3 inTangent: TANGENT0;
    [[vk::location(3)]] float2 inUV: TEXCOORD0;
};

struct CameraUBO {
    float4x4 View;
    float4x4 Proj;
    float4x4 VP;
    float4x4 InvVP;
    float4 Pos;
};

struct ModelUBO {
    float4x4 ModelMat;
    float4x4 ModelInvMat;
};

[[vk::binding(0, 0)]] cbuffer uCamera { CameraUBO uCamera; };
[[vk::binding(0, 1)]] cbuffer uModel { ModelUBO uModel; };

//cbuffer cameraUbo : register(b0, space0) { CameraUBO uCamera; }
//cbuffer modelUbo : register(b1, space0) {ModelUBO uModel;}

VSOutput MainVS(VSInput vIn, uint vertexID: SV_VERTEXID) {
    VSOutput output;
    output.Position = mul(uCamera.VP, mul(uModel.ModelMat, float4(vIn.inPosition.xyz, 1.0)));
    output.worldNormal = mul(transpose((float3x3)uModel.ModelInvMat), vIn.inNormal);
    output.UV = vIn.inUV;
    return output;
}

[[vk::binding(0, 2)]] Texture2D uAlbedoTex;
[[vk::binding(1, 2)]] SamplerState uSampler;

//SamplerState  uSampler: register(s0);
//Texture2D uAlbedoTex: register(t0);

struct PSOutput {
    float4 OutNormal: SV_TARGET0;
    float4 OutAlbedo: SV_TARGET1;
};

PSOutput MainPS(VSOutput pIn) {
    float3 worldNormal01 = pIn.worldNormal * 0.5 + 0.5;
    PSOutput psOut;
    psOut.OutNormal = float4(worldNormal01, 1.0);
    psOut.OutAlbedo = uAlbedoTex.Sample(uSampler, pIn.UV).rgba;
    return psOut;
}