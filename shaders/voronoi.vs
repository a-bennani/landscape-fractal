#version 330

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

layout (location = 0) in vec3 vsiPosition;

out vec3 vsoPosition;

void main(void) {
  vsoPosition = vsiPosition;
  gl_Position = projectionMatrix * modelViewMatrix * vec4(vsiPosition, 1.0);
}
