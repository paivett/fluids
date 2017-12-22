#version 450

uniform mat4 mv_matrix;
uniform mat4 pr_matrix;
uniform float point_radius;
uniform vec4 color;
uniform mat4 inv_projection;
uniform float far_plane;
uniform float near_plane;

// Textures
uniform sampler2D depth_texture;
uniform sampler2D blurred_depth_texture;
uniform sampler2D normals_texture;
uniform sampler2D background_texture;
uniform samplerCube cube_map_texture;

// Inputs of the fragment shader
in vec3 fs_quad_vertex_pos;
in vec2 fs_quad_text_coord;

// Output of the fragment shader
out vec4 fragment_color;

vec3 uv_to_eye(vec2 tex_coord, float depth) {
    float x = tex_coord.x * 2.0 - 1.0;
    float y = tex_coord.y * 2.0 - 1.0;
    vec4 clip_pos = vec4(x , y, depth, 1.0f);
    vec4 view_pos = inv_projection * clip_pos;
    return view_pos.xyz / view_pos.w;
}

void main() {
    vec4 light_dir =  mv_matrix * vec4(0.7f, 1.0f, 0.0f, 0.0f);

    //Get Texture Information about the Pixel
    float depth = texture(blurred_depth_texture, fs_quad_text_coord).r;
    vec3 normal = texture(normals_texture, fs_quad_text_coord).xyz;
    vec3 background_color = texture(background_texture, fs_quad_text_coord).xyz;
    vec3 position = uv_to_eye(fs_quad_text_coord, depth);
    vec3 incident = normalize(light_dir.xyz);
    vec3 viewer = normalize(-position);

    //Blinn-Phong Shading Coefficients
    vec3 h = normalize(incident + viewer);
    float specular = pow(max(0.0f, dot(h, normal)), 50.0f);

    //Background Only Pixels
    if(depth <= 0.0f){
       fragment_color = vec4(background_color, 1);
       return;
    }

    // Fresnel Reflection
    float r_0 = 0.10f;
    float fres_refl = r_0 + (1 - r_0) * pow(1 - dot(normal, viewer), 5.0f);

    // Cube Map Reflection Values
    vec3 reflect_dir = normalize(reflect(-viewer, normal));   
    vec4 refl_color = texture(cube_map_texture, reflect_dir);
    vec3 color_atten = vec3(0.2, 0.2, 0.5);

    //Background Refraction
    vec2 a = fs_quad_text_coord + normal.xy * 0.010f;
    if (texture(blurred_depth_texture, a).r <= 0.0f) {
        a = fs_quad_text_coord;
    }
    vec4 refrac_color = texture(background_texture, a);

    //Final Real Color Mix
    vec3 final_color = mix(color_atten.rgb , (1 - fres_refl) * refrac_color.rgb, 0.7);

    fragment_color = vec4(final_color.rgb + specular * vec3(0.5f) + refl_color.rgb * fres_refl, 1.0f);
}
