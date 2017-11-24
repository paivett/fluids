#version 430 core

in vec2 quad_vertex_pos;
in vec2 quad_text_coord;

// Outputs for the fragment shader
out vec2 fs_quad_text_coord;

void main()
{
    fs_quad_text_coord = quad_text_coord;

    // Quad is rendered in front, no perspective
    gl_Position = vec4(quad_vertex_pos, 0.0f, 1.0f);
}
