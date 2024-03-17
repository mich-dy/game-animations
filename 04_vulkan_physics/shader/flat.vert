#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

layout (location = 0) out vec4 texColor;
layout (location = 1) out vec2 texCoord;
layout (location = 2) out vec3 vertNormal;

layout (set = 1, binding = 0) uniform Matrices {
  mat4 view;
  mat4 projection;
};

layout (std430, set = 2, binding = 0) readonly buffer ModelMatrices {
  mat4 modelMat[];
};

void main() {
  gl_Position = projection * view * modelMat[gl_InstanceIndex] * vec4(aPos, 1.0);
  texColor = vec4(aColor, 1.0);
  texCoord = aTexCoord;
  vertNormal = transpose(inverse(mat3(modelMat[gl_InstanceIndex]))) * aNormal;
}
