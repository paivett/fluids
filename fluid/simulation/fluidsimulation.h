/** 
 *  @file fluidsimulation.h
 *  @brief Contains the declaration of the FluidSimulation class.
 *
 *  This contains the prototype for the FluidSimulation base class. Every
 *  implementation of a fluid solver must inherit from this class.
 *
 *  @author Santiago Daniel Pivetta
 */
 
#ifndef _FLUID_SIMULATION_H_
#define _FLUID_SIMULATION_H_

#include "settings/settings.h"
#include "scene/rigidbody.h"
#include "fluidvolume.h"

/**
 * @class FluidSimulation
 * @brief Base class for any fluid solver implementation
 * @details This is the base class for all fluid solver implementations.
 */
class FluidSimulation {
    public:
        /**
         * Virtual destructor
         */
        virtual ~FluidSimulation() {};

        /**
         * @brief Simulates a delta-t step
         * @details Advances the fluid solver a delta-t step. This is the main
         *          function that the extending class of FluidSimulation must
         *          implement. Basically, here is where the fluid is simulated.
         */
        virtual void simulate() = 0;

        /**
         * @brief Resets the simulation
         * @details Resets the simulation to the begining stage, according to
         *          the settings passed as parameters. The initial codintions
         *          are defined by the solver.
         * 
         * @param fluid_settings Physical properties of the fluid.
         * @param sim_settings Parameters of the simulation.
         */
        virtual void reset(const PhysicsSettings& fluid_settings,
                           const SimulationSettings& sim_settings) = 0;

        /**
         * @brief Returs the number of particles used by the solver.
         * @return The count of particles being used in the simulation.
         */
        virtual int particle_count() const = 0;

        /**
         * @brief Adds a new sampling of a boundary surface
         * 
         * @param boundary An instance of a rigid body that will have a one-way
         *                 or two-way interaction with the fluid, depending 
         *                 on its mass.
         * @param boundary_id A unique identifier for the boundary
         * @param can_move Tells the fluid solver if the boundary can move or 
         *                 not
         */
        virtual void add_boundary(const std::shared_ptr<RigidBody> boundary,
                                  const std::string& boundary_id,
                                  bool can_move=false) = 0;


        /**
         * @brief Sets the limits of a rectangular container where the 
         * simulation takes place
         * 
         * @param width Width of the container
         * @param height Height of the container
         * @param depth Depth of the container
         */
        virtual void set_rect_limits(float width, float height, float depth) = 0;

        /**
         * @brief Returns the number of particles used in the boundary.
         */
        virtual int boundary_particle_count() const = 0;

        /**
         * @brief Adds a new fluid volume to the simulation
         * 
         * @param volume An instance of a fluid volume.
         */
        virtual void add_volume(const std::shared_ptr<FluidVolume> volume) = 0;
};

#endif // _FLUID_SIMULATION_H_