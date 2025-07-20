#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord; // ignored

layout (location = 0) out vec4 lineColor;

layout (std140, binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
};

void main() {
  gl_Position = projection * view * vec4(aPos, 1.0);
  lineColor = vec4(aColor, 1.0);
}
