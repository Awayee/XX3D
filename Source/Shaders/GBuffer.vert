#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;

layout(set = 0, binding = 1) uniform CameraUniform{
    mat4 View;
    mat4 Proj;
    mat4 VP;
    vec3 pos;
}cameraUniform;

layout(set = 1, binding=0)uniform ModelUniform{
    mat4 Model;
}modelUniform;

layout(location = 0) out vec3 worldNormal;
layout(location = 1) out vec2 uv;

void main(){
    gl_Position = cameraUniform.VP * modelUniform.Model * vec4(inPosition, 1.0);
    worldNormal = mat3(transpose(inverse(modelUniform.Model))) * inNormal;
    uv = inUv;
}