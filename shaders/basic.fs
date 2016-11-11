#version 330
uniform int w_one;
uniform vec4 lumpos;
uniform vec4 boule;
uniform sampler2D eau;
uniform sampler1D alt;
uniform mat4 lumModelViewMatrix;
in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec4 vsoModPosition;
in vec3 vsoPosition;

out vec4 fragColor;

void main(void) {
  float amb = 0.2;
  float diff = 0.8;
  float spec = 0.0;
  vec4 lumRot = lumModelViewMatrix * lumpos;
  if(w_one == 0){
    vec3 L = normalize(vsoModPosition.xyz - lumRot.xyz);      
    vec3 R = reflect( L, vsoNormal);
    vec3 V = normalize(-vsoModPosition.xyz);
    //diff = clamp(dot(vsoNormal, -L),0,1);
    spec = max(0, dot(R,V));
    vec4 texel = texture(alt, vsoPosition.y);
    //vec3 lum = normalize(vsoModPosition.xyz - lumpos.xyz);
    fragColor = vec4(vec3(amb + diff * dot(normalize(vsoNormal), -L)), 1.0) * texel;
  }else{
    vec3 L = normalize(vsoModPosition.xyz - lumRot.xyz);      
    vec3 R = reflect( L, vec3(0,1,0));
    vec3 V = normalize(-vsoModPosition.xyz);
    diff = clamp(dot(vec3(0,1,0), -L),0,1);
    spec = max(0, dot(R,V));
    vec4 texel = texture(eau, vsoTexCoord);
    fragColor = vec4( (amb + diff) * texel.xyz, 0.6) + vec4(1.0, 0.65, 0.0, 1.0) * pow(spec, 70);
  }
}
