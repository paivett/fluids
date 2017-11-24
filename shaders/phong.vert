#version 430

uniform mat4 m_matrix;
uniform mat4 v_matrix;
uniform mat4 p_matrix;
uniform mat3 normal_matrix;

in vec3 vertex_coord;
in vec3 vertex_normal;

out vec4 frag_coord;
out vec3 frag_normal;

void main() {
    mat4 mv_matrix = v_matrix * m_matrix;
    
    frag_coord = m_matrix * vec4(vertex_coord, 1.0);
    frag_normal = normal_matrix * vertex_normal;

    // Calculate vertex position in screen space
    gl_Position = p_matrix * mv_matrix * vec4(vertex_coord, 1.0);
}
