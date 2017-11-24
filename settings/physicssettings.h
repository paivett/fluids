#ifndef _PHYSICS_SETTINGS_H_
#define _PHYSICS_SETTINGS_H_

// Fluid physical properties
struct PhysicsSettings {
    // Rest density of the fluid in [Kg/m^3]
    float rest_density;
    // Kinetic viscosity of the fluid in [Pa*seg]
    float k_viscosity;
    // Gravity acceleration
    float gravity;
    // Gas stiffness coeficient in [J]
    float gas_stiffness;
    // Fluids surface tension in [N/m]
    float surface_tension;

    PhysicsSettings()
        : rest_density(998.29),
          k_viscosity(0.0035105),
          gravity(9.8),
          gas_stiffness(3),
          surface_tension(0.0728)
    {}

};

#endif // _PHYSICS_SETTINGS_H_