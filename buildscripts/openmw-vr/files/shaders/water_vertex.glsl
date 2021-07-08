#version 120
    
varying vec3  screenCoordsPassthrough;
varying vec4  position;
varying float linearDepth;
varying vec3  passViewPos;

#include "shadows_vertex.glsl"

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    vec4 viewPos = gl_ModelViewMatrix * gl_Vertex;
    passViewPos = viewPos.xyz;

    mat4 scalemat = mat4(0.5, 0.0, 0.0, 0.0,
                         0.0, -0.5, 0.0, 0.0,
                         0.0, 0.0, 0.5, 0.0,
                         0.5, 0.5, 0.5, 1.0);

    vec4 texcoordProj = ((scalemat) * ( gl_Position));
    screenCoordsPassthrough = texcoordProj.xyw;

    position = gl_Vertex;

    linearDepth = gl_Position.z;

    setupShadowCoords(viewPos, normalize((gl_NormalMatrix * gl_Normal).xyz));
}
