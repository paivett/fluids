#version 430

uniform samplerCube skybox;

in vec3 text_coord;
out vec4 final_color;

void main() {
    final_color = texture(skybox, text_coord);
}
