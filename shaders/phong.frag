#version 430

#define MAX_DIR_LIGHTS  8

struct DirectionalLight {
    vec3 direction;
    vec4 attenuation;
    vec4 diffuse;
    vec4 ambient;
    vec4 specular;
};

// The number of directional lights defined
uniform int dir_light_count;
uniform DirectionalLight dir_light[MAX_DIR_LIGHTS];

// Material colors
uniform vec4 material_ambient;
uniform vec4 material_diffuse;
uniform vec4 material_specular;
uniform float material_shininess;


uniform mat4 m_matrix;
uniform mat4 v_matrix;

// Inverse of view matrix
uniform mat4 inv_v_matrix;

in vec4 frag_coord;
in vec3 frag_normal;

out vec4 final_color;

void main() {
    // Normalized fragment normal
    vec3 normal = normalize(frag_normal);
    vec4 camera_pos = inv_v_matrix * vec4(0, 0, 0, 1);
    vec3 view_dir = normalize((camera_pos - frag_coord).xyz);

    // Compute the color provided by all the directional lights
    for (int i=0; i<dir_light_count; ++i) {
        vec4 attenuation = vec4(1.0, 1.0, 1.0, 1.0);
        vec3 light_dir = -normalize(dir_light[i].direction);
        vec4 diffuse = attenuation * dir_light[i].diffuse * material_diffuse * max(0.0, dot(normal, light_dir));
        vec4 ambient = dir_light[i].ambient * material_ambient;
        vec4 specular = attenuation * dir_light[i].specular * material_specular;
        specular *= pow(max(0.0, dot(reflect(-light_dir, normal), view_dir)), material_shininess);
        final_color += diffuse + ambient + specular;
    }
}
 