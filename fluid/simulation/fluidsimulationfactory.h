#ifndef __FLUID_SIMULATION_FACTORY_H__
#define __FLUID_SIMULATION_FACTORY_H__

#include <memory>
#include "fluid/simulation/wcsphsimluation.h"
#include "fluid/simulation/pcisphsimluation.h"
#include "settings/settings.h"


class FluidSimulationFactory {
    public:

        static std::unique_ptr<FluidSimulation> build_simulation(
            const PhysicsSettings& fluid_settings,
            const SimulationSettings& sim_settings,
            GLuint vbo_fluid_particles) {
            
            switch (sim_settings.sim_method) {
                case SimulationSettings::Method::WCSPH:
                    return std::unique_ptr<WCSPHSimulation>(
                                        new WCSPHSimulation(
                                                fluid_settings,
                                                sim_settings,
                                                vbo_fluid_particles
                                        )
                                    );
                    break;
                case SimulationSettings::Method::PCISPH:
                    return std::unique_ptr<PCISPHSimulation>(
                        new PCISPHSimulation(
                                fluid_settings,
                                sim_settings,
                                vbo_fluid_particles
                        )
                    );
                    break;
                default:
                    throw RunTimeException("Unknown simulation method!");
            }
        }

};

#endif // __FLUID_SIMULATION_FACTORY_H__