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

uniform mat4 mv_matrix;
uniform mat4 pr_matrix;
uniform float point_radius;
uniform vec4 color;

// position of center in eye space
in vec3 pos_eye;

in vec4 vertex_color;
out vec4 frag_color;

void main() {
    //calculate normal from texture coordinates
    vec3 n;
    n.xy = gl_PointCoord.st * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    
    float mag = dot(n.xy, n.xy);
    // kill pixels outside circle
    if (mag > 1.0) {
        discard;
    }
    n.z = sqrt(1.0 - mag);
 
    // Output the fragment color
    frag_color = vec4(0, 0, 0, 1);
    for (int i=0; i<dir_light_count; ++i) {
        vec4 attenuation = vec4(1.0, 1.0, 1.0, 1.0);
        vec3 light_dir = -normalize(dir_light[i].direction);
        vec4 diffuse = attenuation * dir_light[i].diffuse * vertex_color * max(0.0, dot(n, light_dir));
        frag_color += diffuse;
    }
    // float diffuse = max(0, dot(n, light_dir));
    // frag_color = diffuse * vertex_color;
}