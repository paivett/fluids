__kernel void shuffle_buffer_4f(global float4* data,
                                global int* mask,
                                global float4* dest_data,
                                const int total_elements) {
    int global_id = get_global_id(0);

    if (global_id < total_elements) {
        int e_pos = mask[global_id];
        float4 e = data[e_pos];
        dest_data[global_id] = e;
    }
}

__kernel void shuffle_buffer_1f(global float* data,
                                global int* mask,
                                global float* dest_data,
                                const int total_elements) {
    int global_id = get_global_id(0);

    if (global_id < total_elements) {
        int e_pos = mask[global_id];
        float e = data[e_pos];
        dest_data[global_id] = e;
    }
}
