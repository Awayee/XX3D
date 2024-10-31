//vs

struct GBufferData{
    float3 Normal;
    float3 BaseColor;
    float Roughness;
    float Metallic;
};

void EncodeGBuffer(GBufferData data, inout float4 gBuffer0, inout float4 gBuffer1){
    gBuffer0.xyz = data.Normal;
    gBuffer0.w = data.Roughness;
    gBuffer1.xyz = data.BaseColor;
    gBuffer1.w = data.Metallic;
}

void DecodeGBuffer(float4 gBuffer0, float4 gBuffer1, inout GBufferData data) {
    data.Normal = gBuffer0.xyz;
    data.Roughness = gBuffer0.w;
    data.BaseColor = gBuffer1.xyz;
    data.Metallic = gBuffer1.w;
}