#version 460 core
layout (location = 0) in vec4 texColor;
layout (location = 1) in vec2 texCoord; // ignored
layout (location = 2) in vec3 vertNormal;

layout (location = 0) out vec4 FragColor;

vec3 lightPos = vec3(40.0, -50.0, -30.0);
vec3 lightColor = vec3(1.0, 1.0, 1.0);

const float ambientLight = 0.3f;

void main() {
  float lightAngle = max(dot(normalize(vertNormal), normalize(lightPos)), 0.0);
  FragColor = vec4((ambientLight + 0.7 * lightAngle) * lightColor, 1.0) * texColor;
}
