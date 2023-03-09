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

void main(){
    vec4 gNormal = subpassLoad(uNormal).rgba;
    vec4 gAlbedo = subpassLoad(uAlbedo).rgba;
    vec3 lightColor = SceneLightColor.xyz;
    vec3 worldPos = RebuildWorldPos();
    vec3 viewDir = normalize(inCameraTransform.CameraPos - worldPos);
    vec3 lightDir = normalize(-SceneLightDir.xyz);
    vec3 worldNormal = normalize(gNormal.xyz * 2.0 - 1.0);
    vec3 albedo = gAlbedo.rgb;
    vec3 blinnPhong = BlinnPhong(lightDir, viewDir, worldNormal, albedo, lightColor);

    vec3 result = ambient + blinnPhong;
    outColor = vec4(blinnPhong, 1.0);
}