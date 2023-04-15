#version 120

uniform sampler2D diffuseMap;

varying vec2 diffuseMapUV;

void main()
{
    gl_FragColor = texture2D(diffuseMap, diffuseMapUV);
    if(gl_FragColor.a == 0.0) discard;
}
