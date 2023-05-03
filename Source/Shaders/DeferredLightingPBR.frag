/*
#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 outColor;

// layout(set=0, binding=0) uniform sampler uSampler;
// layout(set=0, binding=1) uniform image2D uNormal;
// layout(set=0, binding=2) uniform image2D uAlbedo;
layout(set=0, binding=1) uniform sampler2D uNormal;
layout(set=0, binding=2) uniform sampler2D uAlbedo;

void main(){
    vec4 sampledNormal = texture(uNormal, inUV);
    vec4 sampledAlbedo = texture(uNormal, inUV);
    outColor = vec4(sampledNormal.xyz, 1.0);
}
*/

#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in CameraTansform{
    mat4 InverseVP;
    vec3 CameraPos;
}inCameraTransform;

layout(location = 0) out vec4 outColor;

layout(set=0, binding=1) uniform SceneUniform{
    vec4 SceneLightDir;
    vec4 SceneLightColor;
};

layout(input_attachment_index=0, set=0, binding=2) uniform subpassInput uNormal;
layout(input_attachment_index=1, set=0, binding=3) uniform subpassInput uAlbedo;
layout(input_attachment_index=2, set=0, binding=4) uniform subpassInput uDepth;

const float PI = 3.14159265359;
const float ROUGHNESS = 0.5;
const float METALIC   = 0.1;
const float LIGHT_STRENGTH = 3.0;

// Normal distribution
float DistributionGGX(vec3 N, vec3 H, float roughness){
    float a2 = roughness * roughness;
    float NdotH = max(0, dot(N, H));
    float denomi = (NdotH * NdotH * (a2 - 1.0f) + 1);
    return a2 / (PI * denomi * denomi);
}

// Geometry occlusion
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    roughness += 1;
    float k = roughness * roughness * 0.125f; // direct light
    float NdotV = max(0, dot(N, V));
    float NdotL = max(0, dot(N, L));
    return  NdotV / (NdotV * (1.0f - k) + k)  *  NdotL / (NdotL * (1.0f - k) + k);
}

// fresnel
vec3 fresnelSchlick(vec3 H, vec3 V, vec3 F0){
    float HdotV = max(0, dot(H, V));
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}  

vec3 RebuildWorldPos(){
    // get depth val
    float depth = subpassLoad(uDepth).r;
    // to ndc
    vec2 uv = inUV*2-1;
    vec4 ndcPos = vec4(uv.x, uv.y, depth, 1);
    vec4 worldPos = inCameraTransform.InverseVP * ndcPos;
    return worldPos.xyz / worldPos.w;
}

const float gloss = 16.0;
const vec3 ambient = vec3(0.1, 0.1, 0.1);

vec3 BlinnPhong(vec3 L, vec3 V, vec3 N, vec3 albedo, vec3 light){
    vec3 H = normalize(L + V);
    vec3 diffuse = max(0.0, dot(L, N)) * albedo;
    vec3 specular = pow(max(0.0, dot(N, H)), gloss) * light;
    return diffuse + specular;
}

vec3 PBRDirectLight(vec3 V, vec3 L, vec3 N, vec3 albedo, vec3 irradiance){
     vec3 F0 = vec3(0.04f);
    vec3 H = normalize(L + V);
    float NDF = DistributionGGX(N, H, ROUGHNESS);
    float G = GeometrySmith(N, V, L, ROUGHNESS);
    vec3 F = fresnelSchlick(H, V, F0);
    vec3 Kd = (vec3(1.0) - F) * (1.0 - METALIC);
    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(0, dot(N, V)) * max(0, dot(N, L)) + 0.001;
    vec3 Fs = nominator / denominator;
    float NdotL = max(0, dot(N, L));
    return irradiance * (Kd * albedo / PI + Fs) * NdotL;
}

void main(){
    vec4 gNormal = subpassLoad(uNormal).rgba;
    vec4 gAlbedo = subpassLoad(uAlbedo).rgba;
    vec3 lightColor = SceneLightColor.xyz;
    vec3 worldPos = RebuildWorldPos();
    vec3 V = normalize(inCameraTransform.CameraPos - worldPos);
    vec3 L = normalize(-SceneLightDir.xyz);
    vec3 N = normalize(gNormal.xyz * 2.0 - 1.0);
    vec3 albedo = gAlbedo.rgb;
    
    vec3 Lo = vec3(0.0);
    float attenuation = 1.0;
    vec3 irradiance = LIGHT_STRENGTH * lightColor * attenuation;
    Lo += PBRDirectLight(V, L, N, albedo, irradiance);

    //ambient
    vec3 ambient = vec3(0.03) * albedo * lightColor * 0.01;
    vec3 color = ambient + Lo;

    // //gama correction
    // color = color / (color + vec3(1.0f));
    // color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}