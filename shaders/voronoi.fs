#version 330

uniform vec3 cell[300];
uniform float rand;
in vec3 vsoPosition;
out vec4 fragColor;

void main(void) {
  int i;
  float min = length(cell[0]), l;
  for(i = 1; i < 300; i++){
    l = length(vsoPosition - cell[i] + vec3(rand, -rand, rand));
    if (l < min) min = l;
  }
  fragColor = vec4(vec3(0.95, 0.3, 0.3) + vec3(1.0, 1.0, 0.0) * (min)*2, 1.0);
}
