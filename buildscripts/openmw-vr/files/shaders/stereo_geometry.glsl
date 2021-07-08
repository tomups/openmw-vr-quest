#version 150 compatibility
#extension GL_ARB_viewport_array : require
//#ifdef GL_ARB_gpu_shader5 // Ref: AnyOldName3: This slightly faster path is broken on Vega 56
#if 0
    #extension GL_ARB_gpu_shader5 : enable
    #define ENABLE_GL_ARB_gpu_shader5
#endif
            
#ifdef ENABLE_GL_ARB_gpu_shader5
    layout (triangles, invocations = 2) in;
    layout (triangle_strip, max_vertices = 3) out;
#else
    layout (triangles) in;
    layout (triangle_strip, max_vertices = 6) out;
#endif
            
// Geometry Shader Inputs
@INPUTS
            
// Geometry Shader Outputs
@OUTPUTS
            
// Stereo matrices
uniform mat4 stereoViewMatrices[2];
uniform mat4 stereoViewProjections[2];
            
void perVertex(int vertex, int viewport)
{
    gl_ViewportIndex = viewport;
    // Re-project
    gl_Position = stereoViewProjections[viewport] * vec4(vertex_passViewPos[vertex],1);
    vec4 viewPos = stereoViewMatrices[viewport] * vec4(vertex_passViewPos[vertex],1);
    gl_ClipVertex = vec4(viewPos.xyz,1);
                
    // Input -> output
@FORWARDING
                
    EmitVertex();
}
            
void perViewport(int viewport)
{
    for(int vertex = 0; vertex < gl_in.length(); vertex++)
    {
        perVertex(vertex, viewport);
    }
    EndPrimitive();
}
            
void main() {
#ifdef ENABLE_GL_ARB_gpu_shader5
    int viewport = gl_InvocationID;
#else
    for(int viewport = 0; viewport < 2; viewport++)
#endif
        perViewport(viewport);
            
}