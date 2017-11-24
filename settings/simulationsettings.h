#ifndef _SIMULATION_SETTINGS_H_
#define _SIMULATION_SETTINGS_H_

/**
 * @brief Simulation settings
 * @details Settings related with the simulation itself, rather than
 *                   physical properties of the fluid
 */
struct SimulationSettings {
    // Time step used to simulate
    float time_step;

    // The max (abs) value any of the 3 components of the velocity vector 
    // of a particle can have, before it's clamped to that value
    float max_vel;

    float fluid_particle_radius;
    float fluid_support_radius;

    float boundary_particle_radius;
    float boundary_support_radius;

    enum Method {
        WCSPH,
        PCISPH
    };

    Method sim_method;

    SimulationSettings()
        : time_step(0.01),
          max_vel(50.0),
          fluid_particle_radius(0.008),
          fluid_support_radius(0.032),
          boundary_particle_radius(0.008),
          boundary_support_radius(0.032),
          sim_method(WCSPH)
    {}
};

#endif // _SIMULATION_SETTINGS_H_