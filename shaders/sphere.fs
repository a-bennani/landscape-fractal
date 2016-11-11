#version 330

uniform sampler2D nuage;
uniform sampler2D nuit;
uniform sampler2D boussole;
uniform int w_one;
uniform float sin_state;
in vec2 vsoTexCoord;
out vec4 fragColor;

void main(void) {
  if(w_one == 1){
    vec4 texel1 = texture(nuage, vsoTexCoord);
    vec4 texel2 = texture(nuit, vsoTexCoord);
    fragColor = texel1 * clamp((sin_state*1.5 + 0.4) , 0, 1) + texel2 * clamp((-sin_state*1.5 + 0.7) , 0, 1);//+ texel2 * (sin_state,2 * 0.25 + 0.5, 2);
  }
  if(w_one == 2){
    vec4 texel = texture(boussole, vsoTexCoord);
    fragColor = texel;
  }
}


/*#version 330
  uniform sampler2D jour;
  uniform sampler2D nuit;
  uniform sampler2D gloss;
  uniform sampler2D nuage;
uniform vec4 lumPos;
uniform int w_one;
in  vec3 gsoNormal;
in  vec3 gsoModPos;
in  vec3 gsoColor;
in  vec2 gsoTexCoord;
out vec4 fragColor;

void main(void) {
float amb = 0.2;
float diff = 0.0;
float spec = 0.0;
  vec3 L = normalize(gsoModPos - lumPos.xyz);      
  vec4 luminance = vec4(vec3(dot(gsoNormal, -L)), 1.0)  ;
  vec3 R = reflect(L , gsoNormal);
  vec3 V = normalize(-gsoModPos);
  diff = clamp(dot(gsoNormal, -L),0,1);
  spec = max(0, dot(R,V));
  
  vec4 texel1 = texture2D(jour, gsoTexCoord);
  vec4 texel2 = texture2D(nuit, gsoTexCoord);
  vec4 texel3 = texture2D(gloss, gsoTexCoord);
  vec4 texel4 = texture2D(nuage, gsoTexCoord);
  //vec4 Color = vec4(mix(texel2.rgb, texel1.rgb, diff), 1.0);
  fragColor = (amb + diff) *  texel4 + (0.2 + 0.8 * vec4(texel3.rgb, 1.0)) * pow(spec, 10);
  
}
*/
