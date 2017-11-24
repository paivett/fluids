#version 450

uniform mat4 mv_matrix;
uniform mat4 pr_matrix;
uniform float point_radius;
uniform float point_scale;

// Shader inputs
in vec4 particle_pos;

// Vertex shader outputs sent to the fragment shader
out vec3 pos_eye;
out vec4 vertex_color;

void main()
{
    // Calculate the point size
    pos_eye = vec3(mv_matrix * vec4(particle_pos.xyz, 1.0));
    float dist = length(pos_eye);
    gl_PointSize = point_radius * (point_scale / dist);
    
    // Calculate vertex position in screen space
    gl_Position = pr_matrix * mv_matrix * vec4(particle_pos.xyz, 1);
}
