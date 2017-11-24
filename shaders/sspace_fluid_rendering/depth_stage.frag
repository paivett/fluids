#version 450

uniform mat4 pr_matrix;
uniform float point_radius;

// The reduction factor of the viewport for this stage
uniform float viewport_res_factor;

// Width and height of the background depth texture
uniform float bkg_width;
uniform float bkg_height;
uniform sampler2D bkg_depth_texture;

// position of center in eye space
in vec3 pos_eye;

in vec4 vertex_color;
out float frag_depth;


float linearize_depth(float normalized_depth) {
    float far = gl_DepthRange.far;
    float near = gl_DepthRange.near;
    return (((far - near) * normalized_depth) + near + far) / 2.0;
}


void main() {
    //calculate normal from texture coordinates
    vec3 normal;
    normal.xy = gl_PointCoord.st * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    
    float mag = dot(normal.xy, normal.xy);
    
    // kill pixels outside circle
    if (mag > 1.0f) {
        discard;
    }

    normal.z = sqrt(1.0 - mag);
    normal.y = -normal.y;
    normal = normalize(normal);

    // point on surface of sphere in eye space
    vec4 sphere_pos_eye = vec4(pos_eye + (normal * point_radius), 1.0);

    vec4 clip_space_pos = sphere_pos_eye * pr_matrix;
    float norm_depth = clip_space_pos.z / clip_space_pos.w;

    float linear_depth = linearize_depth(norm_depth);
    vec2 bkg_depth_coord = gl_FragCoord.xy / vec2(bkg_width, bkg_height) * viewport_res_factor;
    float bkg_depth = texture(bkg_depth_texture, bkg_depth_coord).r;

    gl_FragDepth = linear_depth;

    if(linear_depth > bkg_depth) {
        discard;
    }

    frag_depth = norm_depth;
}