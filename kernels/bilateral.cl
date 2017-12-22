__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__constant int window_width = 4;

__kernel void bilateral(__read_only image2d_t input_img,
                        __write_only image2d_t output_img,
                        const int width,
                        const int height) {
    const int2 pos = {get_global_id(0), get_global_id(1)};

    if (pos.x >= width || pos.y >= height) {
        return;
    }

    float depth = read_imagef(input_img, sampler, pos).x;
    float out_depth = 0;

    if (depth > 0.0f) {
        float sum = 0.0f;
        float wsum = 0.0f;

        for(int x = -window_width; x < window_width; ++x) {
            for(int y = -window_width; y < window_width; ++y){
                int2 delta = {x, y};
                
                int2 new_pos = pos + delta;

                float sample_depth = read_imagef(input_img, sampler, pos + delta).x;
                
                if (fabs(sample_depth - depth) < 0.015f) {
                    //Spatial
                    float r = fast_length(convert_float2(delta)) * 0.0001f;
                    float w = exp(-r*r);

                    sum += sample_depth * w;
                    wsum += w;
                }
            }
        }

        // Vary contribution with absolute depth differential - trick from pySPH
        out_depth = sum/wsum;
    }

    float4 out = {out_depth, 0, 0, 0};
    write_imagef(output_img, pos, out);
}
