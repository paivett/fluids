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
kernel void compute_normals(const global float4* positions,
                            const global float* densities,
                            global float4* normals,
                            global int* neigh_list_length,
                            global int* neighbourhood_list,
                            const float w_grad_constant,
                            const float particle_mass,
                            const float support_radius) {
    // Current fluid particle index
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = positions[i];
    float4 normal = {0, 0, 0, 0};

    // Load how many neigbours this particle has
    int list_lenght = neigh_list_length[i];
    
    for (int k = 0; k < list_lenght; ++k) {
        int j = neighbourhood_list[mad24(k, PARTICLE_COUNT, i)];
        float4 pos_j = positions[j];
        float4 r = pos_i - pos_j;

        normal += GRAD_W_DEFAULT(r, length(r), support_radius) / densities[j];
    }

    normals[i] = w_grad_constant * normal * particle_mass * support_radius;
} 
