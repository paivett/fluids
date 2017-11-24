__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__constant float dt = 0.00055f;
__constant float dzt = 1000.0f;


__kernel void curvature_flow(__read_only image2d_t input_img,
                             __write_only image2d_t output_img,
                             const int width,
                             const int height,
                             const float inv_proj_x,
                             const float inv_proj_y) {
    const int2 pos = {get_global_id(0), get_global_id(1)};

    if (pos.x >= width || pos.y >= height) {
        return;
    }

    float depth = read_imagef(input_img, sampler, pos).x;
    float out_depth = 0;

    if (depth > 0.0f) {
        // Width of one pixel
        int2 dx = {1, 0};
        int2 dy = {0, 1};

        // Central z value
        float zc = depth; //texture(particleTexture, pos);

        // Take finite differences
        // Central differences give better results than one-sided here.
        // TODO better boundary conditions, possibly.
        // Remark: This is not easy, get to choose between bad oblique view smoothing
        // or merging of unrelated particles
        float zdxp = read_imagef(input_img, sampler, pos + dx).x;
        float zdxn = read_imagef(input_img, sampler, pos - dx).x;
        float zdx = 0.5f * (zdxp - zdxn);
        zdx = (zdxp == 0.0f || zdxn == 0.0f) ? 0.0f : zdx;

        float zdyp = read_imagef(input_img, sampler, pos + dy).x;
        float zdyn = read_imagef(input_img, sampler, pos - dy).x;
        float zdy = 0.5f * (zdyp - zdyn);
        zdy = (zdyp == 0.0f || zdyn == 0.0f) ? 0.0f : zdy;

        // Take second order finite differences
        float zdx2 = zdxp + zdxn - 2.0f * zc;
        float zdy2 = zdyp + zdyn - 2.0f * zc;

        // Second order finite differences, alternating variables
        float zdxpyp = read_imagef(input_img, sampler, pos + dx + dy).x;
        float zdxnyn = read_imagef(input_img, sampler, pos - dx - dy).x;
        float zdxpyn = read_imagef(input_img, sampler, pos + dx - dy).x;
        float zdxnyp = read_imagef(input_img, sampler, pos - dx + dy).x;
        float zdxy = (zdxpyp + zdxnyn - zdxpyn - zdxnyp) / 4.0f;

        // Projection transform inversion terms
        float cx = 2.0f / (width * -inv_proj_x);
        float cy = 2.0f / (height * -inv_proj_y);

        // Normalization term
        float d = cy * cy * zdx * zdx + cx * cx * zdy * zdy + cx * cx * cy * cy * zc * zc;

        // Derivatives of said term
        float ddx = cy * cy * 2.0f * zdx * zdx2 + cx * cx * 2.0f * zdy * zdxy + cx * cx * cy * cy * 2.0f * zc * zdx;
        float ddy = cy * cy * 2.0f * zdx * zdxy + cx * cx * 2.0f * zdy * zdy2 + cx * cx * cy * cy * 2.0f * zc * zdy;

        // Temporary variables to calculate mean curvature
        float ex = 0.5f * zdx * ddx - zdx2 * d;
        float ey = 0.5f * zdy * ddy - zdy2 * d;

        // Finally, mean curvature
        float h = 0.5f * ((cy * ex + cx * ey) / pow(d, 1.5f));

        float3 dxyz = {zdx, zdy, h};

        // Vary contribution with absolute depth differential - trick from pySPH
        out_depth = depth + dxyz.z * dt * (1.0f + (fabs(dxyz.x) + fabs(dxyz.y)) * dzt);
    }

    float4 out = {out_depth, 0, 0, 0}; 
    write_imagef(output_img, pos, out);
}
