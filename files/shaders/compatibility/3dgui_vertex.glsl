#version 120

#include "lib/core/vertex.h.glsl"

varying vec2 diffuseMapUV;

void main()
{
    gl_Position = modelToClip(gl_Vertex);
    diffuseMapUV = gl_MultiTexCoord0.xy;
}
