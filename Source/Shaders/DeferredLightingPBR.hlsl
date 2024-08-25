//#include "ShadowMap.hlsli"
#define CASCADE_NUM 4
#define SHADOW_VAL 0.1

struct VSOutput {
	float4 Position:SV_POSITION;
	float2 UV: TEXCOORD0;
};
//vs
static float2 g_Vertices[6] = {
	float2(-1.0, -1.0), float2(-1.0,  1.0), float2(1.0, -1.0),
	float2(-1.0,  1.0), float2(1.0,  1.0), float2(1.0, -1.0)
};

static float g_Depth = 0.5;

struct VSInput {};

VSOutput MainVS(VSInput vIn, uint vID: SV_VertexID) {
	float2 v = g_Vertices[vID];
	VSOutput vOut;
	vOut.Position = float4(v.xy, g_Depth, 1.0);
	vOut.UV = v * 0.5 + 0.5;
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
	float2 ndcXY = uv * 2 - 1;
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

float2 GetTexelSize2D(Texture2DArray<float> tex) {
    uint w, h, l, m;
    tex.GetDimensions(0, w, h, l, m);
    return float2(1.0 / (float) w, 1.0 / (float) h);
}

float PCF25(Texture2DArray<float> inShadowMap, SamplerState inSampler, float2 uv, uint cascade, float testZ){
#define PCF_NUM 25
    float2 texelSize = GetTexelSize2D(inShadowMap);
    const float2 PoissonDist[PCF_NUM] = {
        float2(-0.978698, -0.0884121),
	    float2(-0.841121, 0.521165),
	    float2(-0.71746, -0.50322),
	    float2(-0.702933, 0.903134),
	    float2(-0.663198, 0.15482),
	    float2(-0.495102, -0.232887),
	    float2(-0.364238, -0.961791),
	    float2(-0.345866, -0.564379),
	    float2(-0.325663, 0.64037),
	    float2(-0.182714, 0.321329),
	    float2(-0.142613, -0.0227363),
	    float2(-0.0564287, -0.36729),
	    float2(-0.0185858, 0.918882),
	    float2(0.0381787, -0.728996),
	    float2(0.16599, 0.093112),
	    float2(0.253639, 0.719535),
	    float2(0.369549, -0.655019),
	    float2(0.423627, 0.429975),
	    float2(0.530747, -0.364971),
	    float2(0.566027, -0.940489),
	    float2(0.639332, 0.0284127),
	    float2(0.652089, 0.669668),
	    float2(0.773797, 0.345012),
	    float2(0.968871, 0.840449),
	    float2(0.991882, -0.657338)
    };
    float shadowVal = 0.0f;
    const float dilation = 1.0f;
	[unroll]
    for (uint i = 0; i < PCF_NUM; ++i){
        float2 offsetUV = uv + texelSize * PoissonDist[i] * dilation;
        float closestZ = inShadowMap.Sample(inLinearSampler, float3(offsetUV, (float) cascade));
        shadowVal += (1.0 + (SHADOW_VAL - 1.0) * step(closestZ, testZ)); // in shadow distance and current z is greater than closest z.
    }
    return shadowVal / PCF_NUM;
}

float CalcShadowVal(Texture2DArray<float> inShadowMap, SamplerState inSampler, float2 uv, uint cascade, float testZ) {
    float closestZ = inShadowMap.Sample(inLinearSampler, float3(uv, (float) cascade)).r;
    return (1.0 + (SHADOW_VAL - 1.0) * step(closestZ, testZ));
}

float PCFByMicrosoft(Texture2DArray<float> inShadowMap, SamplerState inSampler, float2 uv, uint cascade, float testZ) {
    float2 texelSize = GetTexelSize2D(inShadowMap);
    const float dilation = 1.0f;
    float d1 = dilation * texelSize.x * 0.125;
    float d2 = dilation * texelSize.x * 0.875;
    float d3 = dilation * texelSize.x * 0.625;
    float d4 = dilation * texelSize.x * 0.375;
    float result = (
        2.0 * CalcShadowVal(inShadowMap, inSampler, uv, cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(-d2, d1), cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(-d1, -d2), cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(d2, -d1), cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(d1, d2), cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(-d4, d3), cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(-d3, -d4), cascade, testZ) +
        CalcShadowVal(inShadowMap, inSampler, uv + float2(d4, -d3), cascade, testZ)+
        CalcShadowVal(inShadowMap, inSampler, uv + float2(d4, d3), cascade, testZ)
    ) / 10.0f;
    return result;
}

// compute csm
float ComputeCascadeShadow(float3 worldPos){
	// to view space
    float viewZ = abs(mul(uCamera.View, float4(worldPos, 1.0)).z);
    uint cascade = 0;
	// if out of shadow distance, return no shadow
	if(viewZ > uLight.FarDistances[CASCADE_NUM-1]) {
        return 1.0f;
    }
	[unroll]
    for (uint i = 0; i < CASCADE_NUM; ++i){
        if (viewZ < uLight.FarDistances[i]){
            cascade = i;
            break;
        }
    }
    float4 clipPos = mul(uLight.VPMats[cascade], float4(worldPos, 1.0));
    clipPos.xyz /= clipPos.w;
    float currentZ = clipPos.z;
	if(currentZ > 1.0f) {
        return 1.0f;
    }
    float2 shadowMapUV = clamp(clipPos.xy * 0.5 + 0.5, 0.0f, 1.0f);
    return PCFByMicrosoft(inShadowMap, inLinearSampler, shadowMapUV, cascade, currentZ);
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
    float shadowVal = ComputeCascadeShadow(worldPos);
    color = color * shadowVal;

	//gama correction
    //color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, 1.0 / 2.2);
	PSOutput pOut;
	pOut.OutColor = float4(color, 1.0);
	return pOut;
}
