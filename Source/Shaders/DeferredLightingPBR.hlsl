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
struct CameraUBO {
	float4x4 View;
	float4x4 Proj;
	float4x4 VP;
	float4x4 InvVP;
	float4 Pos;
};
struct LightUBO {
	float4 Dir;
	float4 Color;
};

[[vk::binding(0, 0)]] cbuffer uCamera { CameraUBO uCamera; };
[[vk::binding(1, 0)]] cbuffer uLight { LightUBO uLight; };
[[vk::binding(2, 0)]] Texture2D<float4> inNormal;
[[vk::binding(3, 0)]] Texture2D<float4> inAlbedo;
[[vk::binding(4, 0)]] Texture2D<float> inDepth;
[[vk::binding(5, 0)]] SamplerState inPointSampler;

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

PSOutput MainPS(VSOutput pIn) {
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

	 //gama correction
	 //color = color / (color + float3(1.0, 1.0, 1.0));
	 //color = pow(color, 1.0 / 2.2);
	PSOutput pOut;
	pOut.OutColor = float4(color, 1.0);
	return pOut;
}
