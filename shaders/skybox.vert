#version 430

uniform mat4 mv_matrix;
uniform mat4 pr_matrix;

in vec3 v_position;
out vec3 text_coord;

void main() {
    // Calculate vertex position in screen space
    gl_Position = pr_matrix * mv_matrix * vec4(v_position.xyz, 1);
    text_coord = v_position;
}
