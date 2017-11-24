#include "grid.h"
#include "morton.h"
#include "macros.h"
#include "kernels.h"
#include "surface_tension.h"

/**
 * @brief Computes initial density and pressure for each particle
 * @details For each particle, computes the initial density value. Also
 *          for every particle, initializes the pressure value.
 * 
 * @param fluid_positions A buffer of fluid particle positions, sorted by
 *                        the particle hash.
 * @param fluid_densities A buffer to write each fluid particle mass density.
 * @param fluid_pressures A buffer to write each fluid particle pressure.
 * @param particle_mas The particle mass.
 * @param support_radius The support radius for the smoothing kernels.
 * @param gas_siffness The gas stiffness constant to compute the pressure.
 * @param rest_density The fluid rest density.
 * @param poly6_eval The poly6 kernel constant.
 * @param fluid_neigh_list_lengths A buffer that holds, for each particle, the 
 *                                 length of its fluid neighbourhood.
 * @param fluid_neigh_indices A buffer that holds all fluid neighbourhoods 
 *                            indices
 * @param boundary_neigh_indices A buffer that holds all boundary neighbourhoods 
 *                               indices
 * @param boundary_neigh_list_lengths A buffer that holds, for each particle, 
 *                                    the length of its boundary neighbourhood.
 * @param boundary_positions A list of all boundary particle positions.
 * @param boundary_phis A list of all boundary particles phi constant.
 */
kernel void compute_density_pressure(const global float4* fluid_positions,
                                     global float* fluid_densities,
                                     global float* fluid_pressures,
                                     const float particle_mass,
                                     const float support_radius,
                                     const float gas_stiffness,
                                     const float rest_density,
                                     const float poly6_eval,
                                     const global int* fluid_neigh_list_lengths,
                                     const global int* fluid_neigh_indices,
                                     const global int* boundary_neigh_indices,
                                     const global int* boundary_neigh_list_lengths,
                                     const global float4* boundary_positions,
                                     const global float* boundary_phis) {
    // The id of the current particle
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = fluid_positions[i];
    float density_i = 0.0f;
    float b_density_i = 0.0f;

    int list_lenght = fluid_neigh_list_lengths[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = fluid_neigh_indices[mad24(k, PARTICLE_COUNT, i)];
        float r_norm = distance(pos_i, fluid_positions[j]);
        density_i += W_DEFAULT(r_norm, support_radius);
    }
    density_i *= poly6_eval * particle_mass;

    // Now iterate over the static boundary particles
    list_lenght = boundary_neigh_list_lengths[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = boundary_neigh_indices[mad24(k, PARTICLE_COUNT, i)];
        float r_norm = distance(pos_i, boundary_positions[j]);
        b_density_i += W_DEFAULT(r_norm, support_radius) * boundary_phis[j];
    }
    density_i += poly6_eval * b_density_i;

    // Set density and update pressure
    fluid_densities[i] = density_i;
    // This is the formula proposed in "Weakly compressible SPH"
    fluid_pressures[i] = max(0.0f, gas_stiffness*(pown(density_i/rest_density, 7) - 1.0f));
}

kernel void compute_acceleration(const global float4* fluid_position,
                                 const global float4* fluid_velocity,
                                 const global float4* fluid_normal,
                                 const global float* fluid_density,
                                 const global float* fluid_pressure,
                                 global float4* fluid_acceleration,
                                 const float4 g,
                                 const float particle_mass,
                                 const float support_radius,
                                 const float sqr_support_radius,
                                 const float k_viscosity,
                                 const float surface_tension_coef,
                                 const float rest_density,
                                 const SmoothingConstants sc,
                                 global int* fluid_neigh_list_length,
                                 global int* fluid_neigh_list,
                                 global int* sb_neigh_list,
                                 global int* sb_neigh_list_length,
                                 global float4* sb_position,
                                 global float* sb_phi,
                                 const float st_kernel_main_c,
                                 const float st_kernel_term_c) {
    // Current fluid particle index
    int i = get_global_id(0);

    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    float4 pos_i = fluid_position[i];

    float4 f_cohesion = {0, 0, 0, 0};
    float4 f_curvature = {0, 0, 0, 0};
    float4 pressure_acc = {0, 0, 0, 0};
    float4 viscosity_acc = {0, 0, 0, 0};

    float density_i = fluid_density[i];
    float pressure_i = fluid_pressure[i];
    float4 vel_i = fluid_velocity[i];
    float4 normal_i = fluid_normal[i];
    float C = pressure_i/pown(density_i, 2);
    
    // Acceleration due to boundary interaction
    float4 b_pressure_acc = {0, 0, 0, 0};

    // First, iterate over the static boundary particles to copmute the force 
    // exerted by the boundary particles
    int b_list_lenght = sb_neigh_list_length[i];
    for (int k = 0; k < b_list_lenght; ++k) {
        int j = sb_neigh_list[mad24(k, PARTICLE_COUNT, i)];
        float4 r = pos_i - sb_position[j];
        float rnorm = length(r);

        b_pressure_acc += -sb_phi[j] * r * (sc.spiky_grad/rnorm) * SQR(support_radius - rnorm) * 2 * C;
    }
    
    int list_lenght = fluid_neigh_list_length[i];
    for (int k = 0; k < list_lenght; ++k) {
        int j = fluid_neigh_list[mad24(k, PARTICLE_COUNT, i)];

        float4 vel_j = fluid_velocity[j];
        float4 normal_j = fluid_normal[j];
        float density_j = fluid_density[j];
        float pressure_j = fluid_pressure[j];

        float4 r = pos_i - fluid_position[j];
        float rnorm = length(r);
        float sqr_rnorm = rnorm * rnorm;
        float cube_rnorm = sqr_rnorm * rnorm;

        if (density_i > 0 && density_j > 0 && rnorm > 0.00001) {
            // Pressure force accumulation
            pressure_acc += r * (sc.spiky_grad/rnorm) * SQR(support_radius - rnorm) * (C + pressure_j/SQR(density_j));

            // Viscosity force accumulation
            viscosity_acc += ((vel_j - vel_i) * (support_radius - rnorm)) / density_j;
          
            f_curvature += (normal_i - normal_j) * st_correction_factor(rest_density, density_i, density_j);
            f_cohesion += normalize(r) * st_kernel(rnorm, support_radius, st_kernel_term_c) * st_correction_factor(rest_density, density_i, density_j);
        }
    }

    // We can take out the particle mass term of the sum
    // as every particle has constant mass
    pressure_acc *= -particle_mass;
    viscosity_acc *= sc.visc_lapl * k_viscosity;

    f_cohesion *= st_kernel_main_c;
    float4 acc_surface_tension = -surface_tension_coef * (f_curvature + f_cohesion);

    fluid_acceleration[i] = g + acc_surface_tension + pressure_acc + viscosity_acc + b_pressure_acc;
}


kernel void predict_vel_n_pos(global float4* position,
                              global float4* velocity_t,
                              global float4* velocity_half_t,
                              global float4* acceleration,
                              const float dt,
                              global float4* predicted_position,
                              global float4* predicted_velocity_t,
                              global float4* predicted_velocity_half_t,
                              const float max_vel,
                              const float4 container_limits) {
    // Current particle index
    int i = get_global_id(0);
    
    // Validate that we are not out of bound
    if(i >= PARTICLE_COUNT) {
        return;
    }

    // Leap frog scheme
    float4 pos = position[i];
    float4 vel_half_t = velocity_half_t[i];
    float4 acc = acceleration[i];
    float4 vel_t = velocity_t[i];

    vel_half_t += dt * acc;
    //vel_half_t = clamp(vel_half_t + dt * acc, -max_vel, max_vel);
    pos += dt * clamp(vel_half_t + dt * acc, -max_vel, max_vel);

    // Perform collision detection with the global boundary box
    float vel_norm = length(vel_half_t);
    float4 cp = pos;
    cp.zx = clamp(pos.zx, -container_limits.zx/2.0f, container_limits.zx/2.0f);
    cp.y = clamp(pos.y, -container_limits.y/2.0f, 10.0f); // No upper limit
    float d = length(cp - pos);
    if (d > 0 && vel_norm > 0) {
        // Collision response
        float4 normal = normalize(sign(cp - pos));
        float restitution_coef = 1;
        float restitution = restitution_coef * d / (dt * vel_norm);
        vel_half_t -= (1.0f + restitution)*dot(vel_half_t, normal) * normal;
        pos = cp;
    }
    
    vel_t = vel_half_t + (dt * acc * 0.5f);

    predicted_position[i] = pos;
    predicted_velocity_half_t[i] = clamp(vel_half_t, -max_vel, max_vel);
    predicted_velocity_t[i] = clamp(vel_t, -max_vel, max_vel);
}