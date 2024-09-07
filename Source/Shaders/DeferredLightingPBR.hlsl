#include "ShadowCommon.hlsli"

struct VSOutput {
	float4 Position:SV_POSITION;
	float2 UV: TEXCOORD0;
};
//vs
static float2 g_Vertices[6] = {
	float2(-1.0, -1.0), float2(1.0, -1.0), float2(-1.0,  1.0),
	float2(-1.0,  1.0), float2(1.0, -1.0), float2( 1.0,  1.0),
};

static float g_Depth = 0.5;

struct VSInput {};

VSOutput MainVS(VSInput vIn, uint vID: SV_VertexID) {
	float2 v = g_Vertices[vID];
	VSOutput vOut;
	vOut.Position = float4(v.xy, g_Depth, 1.0);
	vOut.UV = v * 0.5 + 0.5;
    vOut.UV.y = 1.0 - vOut.UV.y; // y-down in texture coords for DX12 and Vulkan
	return vOut;
}

//ps
struct ShadowCameraUBO {
	float4x4 View;
	float4x4 Proj;
	float4x4 VP;
	float4x4 InvVP;
	float4 Pos;
};
struct LightUBO {
	float4 Dir;
	float4 Color;
    float4 FarDistances;
    float4x4 VPMats[CASCADE_NUM];
    float4 ShadowDebug;
};

[[vk::binding(0, 0)]] cbuffer uCamera { ShadowCameraUBO uCamera; };
[[vk::binding(1, 0)]] cbuffer uLight { LightUBO uLight; };
[[vk::binding(2, 0)]] Texture2DArray<float> inShadowMap;
[[vk::binding(3, 0)]] Texture2D<float4> inNormal;
[[vk::binding(4, 0)]] Texture2D<float4> inAlbedo;
[[vk::binding(5, 0)]] Texture2D<float> inDepth;
[[vk::binding(6, 0)]] SamplerState inPointSampler;
[[vk::binding(6, 0)]] SamplerState inLinearSampler;

struct PSOutput {
	half4 OutColor:SV_TARGET;
};

float3 RebuildWorldPos(float2 uv) {
	// get depth val
	float depth = inDepth.Sample(inPointSampler, uv);
	// to ndc
    float2 ndcUV = float2(uv.x, 1.0 - uv.y); // restore to y-up
    float2 ndcXY = ndcUV * 2 - 1;
	float4 ndcPos = float4(ndcXY.x, ndcXY.y, depth, 1);
	float4 worldPos = mul(uCamera.InvVP, ndcPos);
	return worldPos.xyz / worldPos.w;
}

static float PI = 3.14159265359;
static float ROUGHNESS = 0.5;
static float METALIC = 0.1;
static float LIGHT_STRENGTH = 3.0;
static float GLOSS = 16.0;
static float3 AMBIENT = float3(0.1, 0.1, 0.1);

// Normal distribution
float DistributionGGX(float3 N, float3 H, float roughness) {
	float a2 = roughness * roughness;
	float NdotH = max(0, dot(N, H));
	float denomi = (NdotH * NdotH * (a2 - 1.0f) + 1);
	return a2 / (PI * denomi * denomi);
}

// Geometry occlusion
float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
	roughness += 1;
	float k = roughness * roughness * 0.125f; // direct light
	float NdotV = max(0, dot(N, V));
	float NdotL = max(0, dot(N, L));
	return  NdotV / (NdotV * (1.0f - k) + k) * NdotL / (NdotL * (1.0f - k) + k);
}

// fresnel
float3 fresnelSchlick(float3 H, float3 V, float3 F0) {
	float HdotV = max(0, dot(H, V));
	return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

float3 PBRDirectLight(float3 V, float3 L, float3 N, float3 albedo, float3 irradiance) {
	float3 F0 = float3(0.04, 0.04, 0.04);
	float3 H = normalize(L + V);
	float NDF = DistributionGGX(N, H, ROUGHNESS);
	float G = GeometrySmith(N, V, L, ROUGHNESS);
	float3 F = fresnelSchlick(H, V, F0);
	float3 Kd = (float3(1.0, 1.0, 1.0) - F) * (1.0 - METALIC);
	float3 nominator = NDF * G * F;
	float denominator = 4.0 * max(0, dot(N, V)) * max(0, dot(N, L)) + 0.001;
	float3 Fs = nominator / denominator;
	float NdotL = max(0, dot(N, L));
	return irradiance * (Kd * albedo / PI + Fs) * NdotL;
}

// compute csm
float3 ComputeCascadeShadow(float3 worldPos, float3 sceneColor){
	// to view space
    float viewZ = abs(mul(uCamera.View, float4(worldPos, 1.0)).z);
    uint cascade = 0;
	// if out of shadow distance, return no shadow
	if(viewZ > uLight.FarDistances[CASCADE_NUM-1]) {
        return sceneColor;
    }

	[unroll]
    for (uint i = 0; i < CASCADE_NUM; ++i){
        if (viewZ < uLight.FarDistances[i]){
            cascade = i;
            break;
        }
    }
    float4 clipPos = mul(uLight.VPMats[cascade], float4(worldPos, 1.0));
    float3 projCoords = clipPos.xyz / clipPos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y; // y-down in texture coords
	// clip the coords out of view
	if(all(TestRange01(projCoords))) {
        //float shadowVal = PCSS(inShadowMap, inLinearSampler, cascade, projCoords);
        float shadowVal = PCF25(inShadowMap, inLinearSampler, cascade, projCoords, 1.5);
        sceneColor *= shadowVal;
    }

	// debug csm
    if (uLight.ShadowDebug.x > 0.0f){
        const float3 debugColors[CASCADE_NUM] = {
            float3(1.0f, 0.0f, 0.0f),
			float3(0.0f, 1.0f, 0.0f),
			float3(0.0f, 0.0f, 1.0f),
			float3(1.0f, 1.0f, 0.0f)
        };
        sceneColor = lerp(sceneColor, debugColors[cascade], 0.2);
    }
    return sceneColor;
}

PSOutput MainPS(VSOutput pIn){
	float4 gNormal = inNormal.Sample(inPointSampler, pIn.UV).rgba;
	float4 gAlbedo = inAlbedo.Sample(inPointSampler, pIn.UV).rgba;
	float3 lightColor = uLight.Color.xyz;
	float3 worldPos = RebuildWorldPos(pIn.UV);
	float3 V = normalize(uCamera.Pos.xyz - worldPos);
	float3 L = normalize(-uLight.Dir.xyz);
	float3 N = normalize(gNormal.xyz * 2.0 - 1.0);
	float3 albedo = gAlbedo.rgb;

	float3 Lo = float3(0.0, 0.0, 0.0);
	float attenuation = 1.0;
	float3 irradiance = LIGHT_STRENGTH * lightColor * attenuation;
	Lo += PBRDirectLight(V, L, N, albedo, irradiance);

	//ambient
	float3 ambient = albedo * lightColor * 0.01;
	float3 color = ambient + Lo;

	//shadow
    color = ComputeCascadeShadow(worldPos, color);

	//gama correction
    //color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, 1.0 / 2.2);
	PSOutput pOut;
	pOut.OutColor = float4(color, 1.0);
	return pOut;
}
