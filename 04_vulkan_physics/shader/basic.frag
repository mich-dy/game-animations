#version 460 core
layout (location = 0) in vec4 texColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 vertNormal;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D Tex;

vec3 lightPos = vec3(40.0, -50.0, -30.0);
vec3 lightColor = vec3(1.0, 1.0, 1.0);

void main() {
  float lightAngle = max(dot(normalize(vertNormal), normalize(lightPos)), 0.0);
  FragColor = texture(Tex, texCoord) * vec4((0.3 + 0.7 * lightAngle) * lightColor, 1.0) * texColor;
  //FragColor = texture(Tex, texCoord) * vec4((0.3 + 0.7 * lightAngle) * lightColor, 1.0);

}
