#include "grid.h"
#include "morton.h"
#include "macros.h"
#include "kernels.h"
#include "surface_tension.h"

/**
 * @brief Computes initial density for each particle
 * @details For each particle, computes the initial density value. Also
 *          for every particle, initializes the pressure to zero, and 
 *          builds the neighbourhood list for later use.
 * 
 * @param fluid_position A buffer of fluid particle positions, sorted by
 *                       the particle hash.
 * @param fluid_cell_interval A buffer where the i-th position has an 
 *                            interval within the fluid_position buffer. 
 *                            Every particle in the interval belogs to 
 *                            the i-th cell of the uniform grid.
 * @param fluid_density A buffer to write each fluid particle mass density.
 * @param fluid_pressure A buffer to write each fluid particle pressure.
 * @param grid_info A struct with info about the uniform grid.
 * @param fluid_neighlist_length A buffer that holds, for every fluid 
 *                               particle, the length of the neigh list.
 * @param fluid_neighlist A buffer that keeps track of the neighbourhood 
 *                        list for every fluid particle.
 * 
 * ...
 *                                                                      
 * @param w_eval_constant The smoothing constant for the eval kernel.
 * @param particle_mass The fluid particle mass.
 * @param support_radius The kernel support radius.
 */
kernel void compute_initial_density(const global float4* fluid_position,
                                    global float* fluid_density,
                                    global float* fluid_pressure,
                                    global int* fluid_neighlist_length,
                                    global int* fluid_neighlist,
                                    global int* sb_neigh_list,
                                    global int* sb_neigh_list_length,
                                    const global float4* sb_positions,
                                    const global float* sb_phi,
                                    const float w_eval_constant) {
    // Current fluid particle index
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = fluid_position[i];
    float density_i = 0.0f;
    // Density component due to boundary particles
    float b_density_i = 0.0f;

    // Load how many neigbours this particle has
    int list_lenght = fluid_neighlist_length[i];

    // Iterate over the fluid neighbourhood. Every neighbour is always 
    // within the support radius.
    for (int k = 0; k < list_lenght; ++k) {
        int j = fluid_neighlist[mad24(k, PARTICLE_COUNT, i)];
        float r_norm = length(pos_i - fluid_position[j]);
        density_i += W_DEFAULT(r_norm, SUPPORT_RADIUS);
    }

    // Load how many neigbours this particle has
    list_lenght = sb_neigh_list_length[i];

    // Iterate over the fluid neighbourhood. Every neighbour is always 
    // within the support radius.
    for (int k = 0; k < list_lenght; ++k) {
        int j = sb_neigh_list[mad24(k, PARTICLE_COUNT, i)];
        float r_norm = length(pos_i - sb_positions[j]);
        b_density_i += W_DEFAULT(r_norm, SUPPORT_RADIUS) * sb_phi[j];
    }

    fluid_density[i] = w_eval_constant * ((density_i * PARTICLE_MASS) + b_density_i);
}


/**
 * @brief Computes initial forces for each particle
 * @details For each particle, computes the forces due to surface tension 
 *          and viscosity. Initializes the pressure force to zero.
 * 
 * @param fluid_position A buffer of fluid particle positions, sorted by
 *                       the particle hash.
 * @param fluid_velocity A buffer of fluid particle velicities.
 * @param fluid_density A buffer that holds the initial density of every fluid particle.




 * @param fluid_cell_interval A buffer where the i-th position has an 
 *                            interval within the fluid_position buffer. 
 *                            Every particle in the interval belogs to 
 *                            the i-th cell of the uniform grid.
 
 * @param fluid_pressure A buffer to write each fluid particle pressure.
 * @param grid_info A struct with info about the uniform grid.
 * @param fluid_neighlist_length A buffer that holds, for every fluid 
 *                               particle, the length of the neigh list.
 * @param fluid_neighlist A buffer that keeps track of the neighbourhood 
 *                        list for every fluid particle.
 * 
 * ...                       
 *                                                                      
 * @param w_eval_constant The smoothing constant for the eval kernel.
 */
kernel void compute_initial_forces(
    const global float4* fluid_position,
    const global float4* fluid_velocity,
    const global float* fluid_density,
    const global float4* fluid_normal,
    global float4* other_force,
    const float k_viscosity,
    const float surface_tension_coef,
    const global int* neigh_list_length,
    const global int* neighbourhood_list,
    const float w_visc_lapl_constant,
    const float st_kernel_c1,
    const float st_kernel_c2)
{   
    // Current fluid particle index
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = fluid_position[i];

    float4 f_viscosity = {0, 0, 0, 0};
    
    // Surface tension model forces
    float4 f_cohesion = {0, 0, 0, 0};
    float4 f_curvature = {0, 0, 0, 0};

    float density_i = fluid_density[i];
    float4 vel_i = fluid_velocity[i];
    float4 normal_i = fluid_normal[i];

    // Load how many neigbours this particle has
    int list_lenght = neigh_list_length[i];

    for (int k = 0; k < list_lenght; ++k) {
        int j = neighbourhood_list[mad24(k, PARTICLE_COUNT, i)];

        float4 pos_j = fluid_position[j];
        float4 vel_j = fluid_velocity[j];
        float density_j = fluid_density[j];
        float4 normal_j = fluid_normal[j];
        
        float4 r = pos_i - pos_j;
        float r_norm = length(r);

        f_viscosity += (vel_j - vel_i) * (LAPL_W_VISCOSITY(r_norm, SUPPORT_RADIUS) / density_j);

        f_curvature += (normal_i - normal_j) * st_correction_factor(REST_DENSITY, density_i, density_j);
        f_cohesion += normalize(r) * st_kernel(r_norm, SUPPORT_RADIUS, st_kernel_c2) * st_correction_factor(REST_DENSITY, density_i, density_j);
    }

    // We can take out the particle mass term of the sum
    // as every particle has constant mass
    f_viscosity *= k_viscosity * SQR(PARTICLE_MASS) * (w_visc_lapl_constant / density_i);

    f_cohesion *= PARTICLE_MASS * st_kernel_c1;
    float4 f_tension = -surface_tension_coef * PARTICLE_MASS * (f_curvature + f_cohesion);

    other_force[i] = f_viscosity + f_tension;
}

/**
 * @brief Predicts velocity and position of particles.
 * @details For each particle, predicts, given the acceleration, the 
 *          next step velocity and position.
 * 
 * @param position A buffer of fluid particle positions.
 * @param velocity A buffer of fluid particle velocities.
 * @param other_force A buffer of forces that every particle suffers, 
 *                    but that is not due to pressure.
 * @param pressure_force A buffer of the pressure force of each particle.
 * @param predicted_pos A buffer to save fluid particle predicted positions.
 * @param predicted_vel A buffer to save fluid particle predicted velocities.
 * @param dt Delta T.
 * @param particle_mass The mass of a particle.
 * @param g The gravity acceleration.
 */
kernel void predict_vel_n_pos(const global float4* position,
                              const global float4* velocity,
                              const global float4* other_force,
                              const global float4* pressure_force,
                              global float4* predicted_pos,
                              global float4* predicted_vel,
                              const float dt,
                              const float4 g,
                              const float max_vel,
                              const float4 container_limits) {
    // Current fluid particle index
    int i = get_global_id(0);
    
    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos = position[i];
    float4 vel = velocity[i]; 
    float4 acc = g + (pressure_force[i] + other_force[i]) / PARTICLE_MASS;

    vel += clamp(acc * dt, -max_vel, max_vel);
    pos += vel * dt;

    // Perform collision detection with the boundary box
    float vel_norm = length(vel);
    float4 cp = pos;
    cp.zx = clamp(pos.zx, -container_limits.zx/2.0f, container_limits.zx/2.0f);
    cp.y = clamp(pos.y, -container_limits.y/2.0f, 10.0f); // No upper limit
    float d = length(cp - pos);
    if (d > 0 && vel_norm > 0) {
        // Collision response
        float4 normal = normalize(sign(cp - pos));
        float restitution_coef = 1;
        float restitution = restitution_coef * d / (dt * vel_norm);
        vel -= (1.0f + restitution)*dot(vel, normal) * normal;
        pos = cp;
    }

    pos.w = 1.0f;
    vel.w = 0.0f;

    predicted_pos[i] = pos;
    predicted_vel[i] = vel;
}

// Computes initial forces before the pressure correction loop. This kernel
// compute the surface tension force, the viscosity force and initializes 
// the pressure force to zero
kernel void update_pressure(const global float4* particles_predicted_pos,
                            global float* mass_density_variation,
                            global float* particles_pressure,
                            const float density_variation_scaling_factor,
                            global int* neigh_list_length,
                            global int* neighbourhood_list,
                            global int* sb_neigh_list,
                            global int* sb_neigh_list_length,
                            const global float4* sb_positions,
                            const global float* sb_phi,
                            const float w_default_constant) {
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pred_pos_i = particles_predicted_pos[i];
    
    // Predict density
    float pred_density = 0.0f;
    // Predicted density due to boundary particles
    float pred_density_b = 0.0f;

    // Now iterate over the fluid particles
    int list_lenght = neigh_list_length[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = neighbourhood_list[mad24(k, PARTICLE_COUNT, i)];
        float4 pred_pos_j = particles_predicted_pos[j];
        pred_density += W_DEFAULT(length(pred_pos_i - pred_pos_j), SUPPORT_RADIUS); 
    }
    pred_density *= w_default_constant * PARTICLE_MASS;

    list_lenght = sb_neigh_list_length[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = sb_neigh_list[mad24(k, PARTICLE_COUNT, i)];
        float r_norm = length(pred_pos_i - sb_positions[j]);
        pred_density_b += W_DEFAULT(r_norm, SUPPORT_RADIUS) * sb_phi[j];
    }
    pred_density_b *= w_default_constant;

    float density_variation = max(0.f, pred_density + pred_density_b - REST_DENSITY);
    
    mass_density_variation[i] = density_variation;

    // update pressure
    particles_pressure[i] += density_variation * density_variation_scaling_factor;
}

kernel void compute_pressure_force(const global float4* particles_positions,
                                   const global float* mass_density,
                                   const global float* particles_pressure,
                                   global float4* pressure_force,
                                   const GridInfo grid_info,
                                   global int* neigh_list_length,
                                   global int* neighbourhood_list,
                                   global int* sb_neigh_list,
                                   global int* sb_neigh_list_length,
                                   global float4* sb_positions,
                                   global float* sb_phi,
                                   const float w_pressure_grad_constant) {
    size_t i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = particles_positions[i];

    float4 f_pressure = {0, 0, 0, 0};
    float4 f_pressure_b = {0, 0, 0, 0};

    float density_i = mass_density[i];
    float pressure_i = particles_pressure[i];
    float C = pressure_i/SQR(density_i);

    // Load how many neigbours this particle has
    int list_lenght = neigh_list_length[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = neighbourhood_list[mad24(k, PARTICLE_COUNT, i)];

        float4 pos_j = particles_positions[j];
        float density_j = mass_density[j];
        float pressure_j = particles_pressure[j];       
        float4 r = pos_i - pos_j;
        float l = length(r);
        
        if (l >= 1e-5f) {
            f_pressure += (C + pressure_j/SQR(density_j)) * GRAD_W_PRESSURE(r, l, SUPPORT_RADIUS);
        }
    }

    f_pressure *= -SQR(PARTICLE_MASS) * w_pressure_grad_constant;

    // Now compute the force exerted by the boundary
    list_lenght = sb_neigh_list_length[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = sb_neigh_list[mad24(k, PARTICLE_COUNT, i)];
        float4 pos_j = sb_positions[j];

        float4 r = pos_i - pos_j;
        float l = length(r);

        f_pressure_b += sb_phi[j] * (2 * C) * GRAD_W_PRESSURE(r, l, SUPPORT_RADIUS);
    }
    f_pressure_b *= -PARTICLE_MASS * w_pressure_grad_constant;

    pressure_force[i] = f_pressure + f_pressure_b;
}
