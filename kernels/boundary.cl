#include "grid.h"
#include "kernels.h"

kernel void compute_boundary_phi(const global float4* positions,
                                 global write_only float* boundary_phi,
                                 const float rest_density,
                                 const float w_eval_constant,
                                 const float phi_coeff,
                                 const int count) {
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= count) {
        return;
    }

    float4 pos_i = positions[i];
    float delta = 0;

    for (int j=0; j < count; ++j) {
        float4 pos_j = positions[j];
        float4 r = pos_i - pos_j;
        float r_norm2 = dot(r,r);
        
        if (r_norm2 < SQR(SUPPORT_RADIUS)) {
            delta += W_DEFAULT(r_norm2, SUPPORT_RADIUS);
        }
    }

    boundary_phi[i] = (rest_density / (delta * w_eval_constant)) / phi_coeff;
}

kernel void transform_particles(const global float4* raw_positions,
                                global float4* new_positions,
                                global float4* velocities,
                                const float4 m0,
                                const float4 m1,
                                const float4 m2,
                                const float4 cm_linear_vel,
                                const float4 cm_angular_vel,
                                const float4 cm_position,
                                const int count) {
    size_t i = get_global_id(0);

    if (i >= count) {
        return;
    }

    // Translate and rotate position
    float4 p = raw_positions[i];
    p = (float4) (dot(m0, p), dot(m1, p), dot(m2, p), 1);
    p += cm_position;
    p.w = 1.0f;
    new_positions[i] = p;

    // Compute particle velocity, given the cm position, and its linear and
    // angular velocity
    velocities[i] = cm_linear_vel + cm_angular_vel * (p - cm_position);
}

kernel void compute_fluid_force(global float4* boundary_positions,
                                global float* boundary_phi,
                                read_only image1d_buffer_t fluid_positions,
                                read_only image1d_buffer_t fluid_densities,
                                read_only image1d_buffer_t fluid_pressures,
                                const global int2* fluid_cell_intervals,
                                const float particle_mass,
                                const float spiky_grad,
                                const GridInfo grid_info,
                                const int particle_count,
                                global float4* fluid_force,
                                global float4* fluid_torque,
                                const float4 center,
                                read_only image1d_buffer_t fluid_velocities,
                                const global float4* boundary_velocities,
                                const float visc_lapl) {
    size_t i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= particle_count) {
        return;
    }

    // Position of the boundary particle
    float4 pos_i = boundary_positions[i];
    float phi_i = boundary_phi[i];
    //float4 vel_i = boundary_velocities[i];
    int4 cell_coord = get_grid_coordinates(&pos_i, &grid_info);

    float4 f_pressure = {0, 0, 0, 0};
    //float4 f_viscosity = {0, 0, 0, 0};

    int neigh_count = 0;
    for (int offset=0; (neigh_count < NEIGH_LIST_MAX_LENGTH) && offset<27; ++offset) {
        // Search for particles in the grid's cell
        int4 neigh_cell_coord = cell_coord + CELL_NEIGH_OFFSET[offset];
        int2 interval = fluid_cell_intervals[CELL_ID(neigh_cell_coord.x, neigh_cell_coord.y, neigh_cell_coord.z, grid_info)];
        for (int j=interval.x; (neigh_count < 50) && (j < interval.y); ++j) {
            float4 pos_j = read_imagef(fluid_positions, j);
            float4 vel_j = read_imagef(fluid_velocities, j);
            float4 r = pos_j - pos_i;
            float rnorm = fast_length(r);
            
            if (rnorm < SUPPORT_RADIUS) {
                float fluid_density = read_imagef(fluid_densities, j).x; 
                float C = read_imagef(fluid_pressures, j).x / SQR(fluid_density);
                ++neigh_count;
                f_pressure += r * (spiky_grad/rnorm)*SQR(SUPPORT_RADIUS-rnorm) * 2 * C;
                //f_viscosity += ((vel_j - vel_i) * (SUPPORT_RADIUS-rnorm)) / SQR(fluid_density);
            }
        }
    }

    f_pressure *= particle_mass * phi_i;
    //float4 f = f_pressure + (f_viscosity * visc_lapl * phi_i * particle_mass);
    fluid_force[i] = f_pressure;
    fluid_torque[i] = cross((pos_i - center), f_pressure);
}