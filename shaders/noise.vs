#version 330
layout (location = 0) in vec4 position;
layout (location = 1)in vec3 normal;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

out float LightIntensity;
out vec3  MCposition;

uniform highp vec4 lightPosition;
uniform float Scale;

void main(void)
{
    
    vec3 LightPosition = vec3(lightPosition);
    
    vec4 ECposition = modelViewMatrix * position;
    MCposition      = vec3(position) * 1.0;
    vec3 tnorm      = normalize(vec3(modelViewMatrix * vec4(normal.xyz, 0.0)).xyz);
    LightIntensity  = dot(normalize(LightPosition - vec3 (ECposition)), tnorm) * 1.5;
    gl_Position     = projectionMatrix * modelViewMatrix * position;
    
}
