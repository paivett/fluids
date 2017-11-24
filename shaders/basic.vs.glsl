#version 430

uniform mat4 mv_matrix;
uniform mat4 pr_matrix;
uniform vec4 color;

in vec4 v_position;
out vec4 v_color;

void main()
{
    // Calculate vertex position in screen space
    gl_Position = pr_matrix * mv_matrix * vec4(v_position.xyz, 1);
    v_color = color;
}
