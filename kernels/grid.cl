#include "grid.h"

/**
 * @brief Computes the cell hash for each particle
 * @details For each particle, finds the cell the particle is in, and
 *          the the cell id (x,y,z) computes the cell hash, that is
 *          stored in the particle_hashes buffer
 *          Also, initializes the mask buffer for later use 
 *          (it is faster to initialize it here than doing a copy 
 *          buffer later)
 * 
 * @param positions A buffer of particle positions
 * @param hashes A buffer to write each particle hash
 * @param mask A buffer to write each particle mask id
 * @param grid_info A struct with info about the grid
 * @param particle_count The total number of particles
 */
kernel void compute_hashes(const global float4* positions,
                           global uint* hashes,
                           global int* mask,
                           const GridInfo grid_info,
                           const int particle_count) {
    int i = get_global_id(0);
    
    // Validate that we are not out of bound
    if(i >= particle_count) {
        return;
    }

    float4 pos = positions[i];
    int4 cell = get_grid_coordinates(&pos, &grid_info);

    hashes[i] = CELL_HASH(cell.x, cell.y, cell.z, grid_info);

    // Initialize mask for later use
    mask[i] = i;
}

/**
 * @brief For each cell, computes the interval within positions buffer
 * @details This kernel computes, for every cell of the uniform grid,
 *          an interval [a,b), where "a" is the index of the first particle,
 *          in the sorted particles buffer, that belongs to the cell, and b
 *          is the first particle that does not belong.
 *          All this indices are stored in the cell_intervals.
 *          If a cell has no particles, the interval is [0,0)
 * @note cell_intervals is assumed to be "clean", meaning that has
 *       been initialized with 0's completely
 *
 * @param positions The buffer of particle positions
 * @param hashes The buffer of particle hashes
 * @param cell_intervals The buffer of the intervals for each cell
 * @param local_positions A local memory buffer for storing positions
 * @param local_hashes A local memory buffer for storing hashes
 * @param grid_info A struct with info about the grid
 * @param particle_count The total number of particles
 */
kernel void compute_cell_intervals(const global float4* positions,
                                   const global uint* hashes,
                                   global int* cell_intervals,
                                   local float4* local_positions,
                                   local uint* local_hashes,
                                   const GridInfo grid_info,
                                   const int particle_count) {
    size_t global_id = get_global_id(0);
    size_t local_id = get_local_id(0);

    // Validate that we are not out of bound
    if(global_id >= particle_count) {
        return;
    }

    // Load current particle position and hash
    uint h0 = hashes[global_id];
    float4 pos0 = positions[global_id];

    // And save them into local mem for later use
    local_positions[local_id] = pos0;
    local_hashes[local_id] = h0;

    barrier(CLK_LOCAL_MEM_FENCE);

    // Calculate the cell id of current particle
    int cell_id_0 = compute_cell_id(&pos0, &grid_info);
    
    // We look the next particle in the sorted particles buffer.
    // If the next particle has different hash, it means it belongs
    // to another cell, so the particle this work item is processing
    // is the last to belong to the cell, so we update the end of the 
    // current cell interval accordingly. The same with the beginning
    // of the cell of the next particle

    if (global_id == particle_count-1) {
        // But if this is the last particle in the buffer, that's it
        // there is no particle to look ahead, the interval ends here
        cell_intervals[2*cell_id_0 + 1] = global_id + 1;
    }
    else {
        uint h1;
        float4 pos1;
        // Load the hash and position of the next particle.
        if (local_id < get_local_size(0)-1) {
            h1 = local_hashes[local_id + 1];
            pos1 = local_positions[local_id + 1];
        }
        else {
            // Load from global memory if this work item is the "last"
            // of the work group (local memory ends here)
            h1 = hashes[global_id + 1];
            pos1 = positions[global_id + 1];
        }

        int cell_id_1 = compute_cell_id(&pos1, &grid_info);

        if (h0 < h1) {
            // Hashes are different, update intervals
            cell_intervals[2*cell_id_0 + 1] = global_id + 1;
            cell_intervals[2*cell_id_1] = global_id + 1;
        }
    }
} 

/**
 * @brief For each particle, generates a neighbourhood list
 * @details This kernel computes, for every particle in the reference 
 *          buffer, a neighbourhood list. This list is made from particles 
 *          in the neigh_positions buffer.
 *
 * @param ref_positions The buffer of reference particle positions
 * @param neigh_positions The buffer of particles to build the neighbourhood 
 *                        list from. This buffer must be sorted by the 
 *                        particle hash.
 * @param cell_intervals The buffer of the intervals for each cell. The 
 *                       interval is for the neigh_positions buffer.
 * @param neigh_list A buffer to write the neigh list for every ref particle.
 * @param neigh_list_lenth A buffer to write the length of the neigh list 
 *                         for every ref particle.
 * @param grid_info A struct with info about the grid.
 * @param sqr_support_radius The sqr of the support radius.
 * @param list_max_length The max length of each neigh list.
 * @param particle_count The total number of particles.
 */
kernel void compute_neigh_list(const global float4* ref_positions,
                               const global float4* neigh_positions,
                               const global int2* cell_interval,
                               global int* neigh_list,
                               global int* neigh_list_length,
                               const GridInfo grid_info,
                               const float sqr_support_radius,
                               const int list_max_length,
                               const int particle_count,
                               local float4* local_positions) {
    // Current particle index
    int i = get_global_id(0);
    int local_id = get_local_id(0);

    int local_lower_bound = get_local_size(0) * get_group_id(0);
    int local_upper_bound = get_local_size(0) * (get_group_id(0) + 1);

    // Validate that we are not out of bound
    if(i >= particle_count) {
        return;
    }

    float4 pos_i = ref_positions[i];
    int4 cell_coord = get_grid_coordinates(&pos_i, &grid_info);
    int list_length = 0;

    // And save them into local mem for later use
    local_positions[local_id] = pos_i;

    barrier(CLK_LOCAL_MEM_FENCE);

    for (int offset=0; (list_length < list_max_length) && offset<27; ++offset) {
        // Search for particles in the grid's cell
        int4 neigh_cell_coord = cell_coord + CELL_NEIGH_OFFSET[offset];
        int2 interval = cell_interval[CELL_ID(neigh_cell_coord.x, neigh_cell_coord.y, neigh_cell_coord.z, grid_info)];
        for (int j=interval.x; (list_length < list_max_length) && (j < interval.y); ++j) {
            float4 pos_j;
            if (local_lower_bound <= j && j < local_upper_bound)
                pos_j = local_positions[j - local_lower_bound];
            else
                pos_j = neigh_positions[j];

            float4 r = pos_i - pos_j;
            float r2 = dot(r, r);
            if (r2 < sqr_support_radius) {
                // Build the neigh list
                neigh_list[mad24(list_length, particle_count, i)] = j;
                ++list_length;
            }
        }
    }

    // Update the length of the neighbourhood
    neigh_list_length[i] = list_length;
}