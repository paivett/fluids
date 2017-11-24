#version 430

uniform mat4 mv_matrix;
uniform mat4 pr_matrix;
uniform vec4 color;
uniform float point_radius;
uniform float point_scale;

out vec3 pos_eye; // Will be sent to the fragment shader interpolated

in vec4 particle_pos;
out vec4 vertex_color;

void main()
{
    // Calculate the point size
    pos_eye = vec3(mv_matrix * vec4(particle_pos.xyz, 1.0));
    float dist = length(pos_eye);
    gl_PointSize = point_radius * (point_scale / dist);
    
    // Calculate vertex position in screen space
    gl_Position = pr_matrix * mv_matrix * vec4(particle_pos.xyz, 1);
    
    // Output the color
    vertex_color = vec4(89/256.0, 152/256.0, 255/256.0, 1);
}
