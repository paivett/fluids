#version 430 core

uniform float far_plane;
uniform float near_plane;
uniform float screen_width;
uniform float screen_height;
//uniform mat4 pr_matrix;

uniform sampler2D depth_texture;

in vec2 fs_quad_text_coord;
out vec4 frag_depth;

// Depth used in the Z buffer is not linearly related to distance from camera
// This restores linear depth
float linearize_depth(float exp_depth, float near, float far) {
    return  (2 * near) / (far + near -  exp_depth * (far - near)); 
}

// // Mean curvature. From "Screen Space Fluid Rendering with Curvature Flow"
// vec3 mean_curvature(vec2 pos) {
//     // Width of one pixel
//     vec2 dx = vec2(1.0f / screen_width, 0.0f);
//     vec2 dy = vec2(0.0f, 1.0f / screen_height);

//     // Central z value
//     float zc =  texture(depth_texture, pos).r;

//     // Take finite differences
//     // Central differences give better results than one-sided here.
//     // TODO better boundary conditions, possibly.
//     // Remark: This is not easy, get to choose between bad oblique view smoothing
//     // or merging of unrelated particles
//     float zdxp = texture(depth_texture, pos + dx).r;
//     float zdxn = texture(depth_texture, pos - dx).r;
//     float zdx = 0.5f * (zdxp - zdxn);
//     zdx = (zdxp == 0.0f || zdxn == 0.0f) ? 0.0f : zdx;

//     float zdyp = texture(depth_texture, pos + dy).r;
//     float zdyn = texture(depth_texture, pos - dy).r;
//     float zdy = 0.5f * (zdyp - zdyn);
//     zdy = (zdyp == 0.0f || zdyn == 0.0f) ? 0.0f : zdy;

//     // Take second order finite differences
//     float zdx2 = zdxp + zdxn - 2.0f * zc;
//     float zdy2 = zdyp + zdyn - 2.0f * zc;

//     // Second order finite differences, alternating variables
//     float zdxpyp = texture(depth_texture, pos + dx + dy).r;
//     float zdxnyn = texture(depth_texture, pos - dx - dy).r;
//     float zdxpyn = texture(depth_texture, pos + dx - dy).r;
//     float zdxnyp = texture(depth_texture, pos - dx + dy).r;
//     float zdxy = (zdxpyp + zdxnyn - zdxpyn - zdxnyp) / 4.0f;

//     // Projection transform inversion terms
//     float cx = 2.0f / (screen_width * -pr_matrix[0][0]);
//     float cy = 2.0f / (screen_height * -pr_matrix[1][1]);

//     // Normalization term
//     float d = cy * cy * zdx * zdx + cx * cx * zdy * zdy + cx * cx * cy * cy * zc * zc;
    
//     // Derivatives of said term
//     float ddx = cy * cy * 2.0f * zdx * zdx2 + cx * cx * 2.0f * zdy * zdxy + cx * cx * cy * cy * 2.0f * zc * zdx;
//     float ddy = cy * cy * 2.0f * zdx * zdxy + cx * cx * 2.0f * zdy * zdy2 + cx * cx * cy * cy * 2.0f * zc * zdy;

//     // Temporary variables to calculate mean curvature
//     float ex = 0.5f * zdx * ddx - zdx2 * d;
//     float ey = 0.5f * zdy * ddy - zdy2 * d;

//     // Finally, mean curvature
//     float h = 0.5f * ((cy * ex + cx * ey) / pow(d, 1.5f));
    
//     return(vec3(zdx, zdy, h));
// }

void main()
{       
    //float depth = linearize_depth(texture(depth_texture, fs_quad_text_coord).r, near_plane, far_plane);
    float depth = texture(depth_texture, fs_quad_text_coord).r;

    // if(depth == 0.0f) {
    //     //frag_color = vec4(0.0f);
    //     gl_FragDepth = 0.5;
    // }
    // else {
    //     const float dt = 0.00055f;
    //     const float dzt = 1000.0f;
    //     vec3 dxyz = mean_curvature(fs_quad_text_coord);

    //     // Vary contribution with absolute depth differential - trick from pySPH
    //     float c = depth + dxyz.z * dt * (1.0f + (abs(dxyz.x) + abs(dxyz.y)) * dzt);
    //     //frag_color = vec4(c);
    //     gl_FragDepth = c;
    // }

    //Get Depth Information about the Pixel
    // float exp_depth = texture(depth_texture,fs_quad_text_coord).r;
    // float lin_depth = linearize_depth(exp_depth, near_plane, far_plane);
    

    float lin_depth = linearize_depth(depth, near_plane, far_plane);
    float blur_radius = (1.0f/lin_depth) * 0.0002f;
    int window_width = 5;
    float sum = 0;
    float wsum = 0;

    if(depth == 0f){
        frag_depth = vec4(depth);
        return;
    }

    for(int x = -window_width; x < window_width; x++){
        for(int y = -window_width; y < window_width; y++){
            vec2 s = vec2(fs_quad_text_coord.s + x*blur_radius, fs_quad_text_coord.t + y*blur_radius);
            float sample_depth = texture(depth_texture, s).r;
            
            if(sample_depth > 0){
                //Spatial
                float r = length(vec2(x,y)) * 0.0001f;
                float w = exp(-r*r);

                sum += sample_depth * w;
                wsum += w;
            }
        }
    }
    
    if(wsum > 0.0f){
        sum = sum/wsum;
    }
    
    if (sum > 0.001f) {
        frag_depth = vec4(sum);
    }
    else {
        frag_depth = vec4(-1.0);
    }
}
