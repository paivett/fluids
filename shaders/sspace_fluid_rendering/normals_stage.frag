#version 450

uniform mat4 inv_projection;
uniform sampler2D depth_texture;

in vec2 fs_quad_text_coord;
out vec4 frag_color;

vec3 uv_to_eye(vec2 tex_coord, float depth) {
    float x = tex_coord.x * 2.0 - 1.0;
    float y = tex_coord.y * 2.0 - 1.0;
    vec4 clip_pos = vec4(x , y, depth, 1.0f);
    vec4 view_pos = inv_projection * clip_pos;
    return view_pos.xyz / view_pos.w;
}

void main() {
    //Get Depth Information about the Pixel
    float depth = texture(depth_texture, fs_quad_text_coord).r;

    if (depth <= 0.0f) {
        discard;
    }

    vec2 texel_size = 1.0f / textureSize(depth_texture, 0);

    vec3 position = uv_to_eye(fs_quad_text_coord, depth);
    vec2 ddx_tex_coord = fs_quad_text_coord + vec2(texel_size.x, 0);
    vec2 ddx2_tex_coord = fs_quad_text_coord + vec2(-texel_size.x, 0);
    vec3 ddx = uv_to_eye(ddx_tex_coord, texture(depth_texture, ddx_tex_coord).r) - position;
    vec3 ddx2 = -uv_to_eye(ddx2_tex_coord, texture(depth_texture, ddx2_tex_coord).r) + position;;
    if (abs(ddx.z) > abs(ddx2.z)) {
        ddx = ddx2;
    }
    
    vec2 ddy_tex_coord = fs_quad_text_coord + vec2(0, texel_size.y);
    vec2 ddy2_tex_coord = fs_quad_text_coord + vec2(0, -texel_size.y);
    vec3 ddy = uv_to_eye(ddy_tex_coord, texture(depth_texture, ddy_tex_coord).r) - position;
    vec3 ddy2 = -uv_to_eye(ddy2_tex_coord, texture(depth_texture, ddy2_tex_coord).r) + position;;
    if (abs(ddy.z) > abs(ddy2.z)) {
        ddy = ddy2;
    }

    frag_color = vec4(normalize(cross(ddx, ddy)), 1.0f);
}
