#version 330

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform int w_one;
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec2 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;
out vec2 vsoTexCoord;
void main(void) {
  if(w_one==1)
    vsoTexCoord = vec2(1.0 - vsiTexCoord.x, vsiTexCoord.y * 2);
  if(w_one==2)
    vsoTexCoord = vec2(1.0 - vsiTexCoord.x, vsiTexCoord.y );
  if(w_one==0)
    vsoTexCoord = vsiTexCoord;
  if(w_one==2)
    gl_Position =  modelViewMatrix * vec4(vsiPosition, 1.0);
else
    gl_Position = projectionMatrix * modelViewMatrix * vec4(vsiPosition, 1.0);
}
/*#version 330

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform sampler2D bump;
uniform int ng;
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec2 vsiTexCoord;
 
out vec3 vsoNormal;
out vec3 vsoPosition;
out vec3 vsoModPos;
out vec2 vsoTexCoord;

void main(void) {
  vsiTexCoord = vec2(1.0 - vsiTexCoord.s, 1.0 - vsiTexCoord.t);
  vec4 texel3 = texture2D(bump, vsiTexCoord);
  vec4 mp;
  mp = modelViewMatrix * vec4(vsiPosition * 1.05, 1.0);
  vsoNormal = (transpose(inverse(modelViewMatrix))  * vec4(vsiPosition, 0.0)).xyz;
  vsoPosition = vsiPosition;
  vsoModPos = mp.xyz;
  vsoTexCoord = vsiTexCoord;
  gl_Position = projectionMatrix * mp;
}
*/
