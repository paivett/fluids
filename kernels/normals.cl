#include "grid.h"
#include "kernels.h"

/**
 * @brief Computes particles normals
 * @details Computes particles normals according the 
 *          algorithm described in 
 *          "Versatile Surface Tension and Adhesion for SPH Fluids"
 * 
 * @param positions A buffer of particle positions.
 * @param densities A buffer of particle densities.
 * @param normals A buffer to write the particle normals.
 * @param grid_info A struct with info about the uniform grid.
 * @param neigh_list_length A buffer with the length of the 
 *                          neighbourhood of each particle.
 * @param neighbourhood_list A buffer with the list of 
 *                           neighbours of each particle.
 * @param w_grad_constant The gradient constant of the default 
 *                        smoothing kernel.
 * @param particle_mass The mass of a particle.
 * @param support_radius The smoothing support radius.
 */
kernel void compute_normals(read_only image1d_buffer_t positions,
                            read_only image1d_buffer_t densities,
                            global float4* normals,
                            const global int* neigh_list_length,
                            const global int* neighbourhood_list,
                            const float w_grad_constant,
                            local float4* pos_cache,
                            local float* dens_cache) {
    // Current fluid particle index
    int i = get_global_id(0);
    int local_id = get_local_id(0);
    int local_lower_bound = get_local_size(0) * get_group_id(0);
    int local_upper_bound = get_local_size(0) * (get_group_id(0) + 1);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = read_imagef(positions, i);
    float4 normal = {0, 0, 0, 0};

    pos_cache[local_id] = pos_i;
    dens_cache[local_id] = read_imagef(densities, i).x;
    barrier(CLK_LOCAL_MEM_FENCE);

    // Load how many neigbours this particle has
    int list_lenght = neigh_list_length[i];

    for (int k = 0; k < list_lenght; ++k) {
        int j = neighbourhood_list[mad24(k, PARTICLE_COUNT, i)];
        float4 pos_j;
        float d;
        if (local_lower_bound <= j && j < local_upper_bound) {
            pos_j = pos_cache[j - local_lower_bound];
            d = dens_cache[j - local_lower_bound];
        }
        else { 
            pos_j = read_imagef(positions, j);
            d = read_imagef(densities, j).x;
        }
        float4 r = pos_i - pos_j;

        normal += GRAD_W_DEFAULT(r, dot(r,r), SUPPORT_RADIUS) / d;
    }

    normals[i] = w_grad_constant * normal * PARTICLE_MASS * SUPPORT_RADIUS;
} 
