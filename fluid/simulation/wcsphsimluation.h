/** 
 *  @file wsphsimulation.h
 *  @brief Contains the declaration of the WSPHSimulation class.
 *
 *  This contains the prototype for the WSPHSimulation class. This solver 
 *  implements the Weakly Compressible SPH.
 *  
 *  @author Santiago Daniel Pivetta
 */

#ifndef _WSPH_SIMULATION_H_
#define _WSPH_SIMULATION_H_

#include <vector>
#include <memory>
#include "opencl/clenvironment.h"
#include "opencl/clprogram.h"
#include "opengl/openglfunctions.h"
#include "fluidsimulation.h"
#include "grid.h"
#include "boundaryhandler.h"
#include "kernels/common.h"

/**
 * @class WCSPHSimulation
 * @brief Implementation based on Weakly Compressible SPH
 * @details This implementation is based on a Weakly Compressible SPH 
 *          solver. For further details
 *          - "Particle-based fluid simulation for interactive applications"
 *          - "Weakly compressible SPH for free surface flows"
 * 
 */
class WCSPHSimulation : public FluidSimulation {
    public:
        /**
         * @brief Constructor
         * @details Creates a new solver that implements a solver based on 
         *          Weakly compressible SPH
         * 
         * @param fluid_settings Physical settings (fluid settings)
         * @param sim_settings Settings related with simulation (particles, etc)
         * @param vbo_fluid_particles OpenGL VBO to hold fluid particles
         */
        WCSPHSimulation(const PhysicsSettings& fluid_settings,
                        const SimulationSettings& sim_settings,
                        GLuint vbo_fluid_particles);

        /**
         * @brief Destructor
         * @details Releases all [long description]
         */
        ~WCSPHSimulation();

        /**
         * @brief Simulates a delta-t step of the wcsph solver
         * @details Advances the fluid solver a delta-t step. This is the main
         *          function of the solver.
         */
        void simulate();

        /**
         * @brief Resets the simulation
         * @details Resets the simulation to the begining stage, according to
         *          the settings passed as parameters. The initial codintions
         *          are defined by the solver.
         * 
         * @param fluid_settings Physical properties of the fluid.
         * @param sim_settings Parameters of the simulation.
         */
        void reset(const PhysicsSettings& fluid_settings,
                   const SimulationSettings& sim_settings);

        /**
         * @brief Returs the number of particles used by the solver.
         * @return The count of particles being used in the simulation.
         */
        int particle_count() const;

        /**
         * @brief Adds a new sampling of a boundary surface
         * 
         * @param boundary An instance of a rigid body that will have a one-way
         *                 or two-way interaction with the fluid, depending 
         *                 on its mass.
         * @param boundary_id A unique identifier for the boundary
         */
        void add_boundary(const std::shared_ptr<RigidBody> boundary,
                          const std::string& boundary_id,
                          bool can_move=false);

        /**
         * @brief Returns the number of particles used in the boundary.
         */
        int boundary_particle_count() const;

        /**
         * @brief Adds a new fluid volume to the simulation
         * 
         * @param volume An instance of a fluid volume.
         */
        void add_volume(const std::shared_ptr<FluidVolume> volume);

        /**
         * @brief Sets the limits of a rectangular container where the 
         * simulation takes place
         * 
         * @param width Width of the container
         * @param height Height of the container
         * @param depth Depth of the container
         */
        void set_rect_limits(float width, float height, float depth);

    private:
        /* Uniform grid */
        std::unique_ptr<Grid> _grid;

        /* Boundary handler */
        std::unique_ptr<BoundaryHandler> _boundary_handler;

        /* A collection of fluid volumes */
        std::vector<std::shared_ptr<FluidVolume> > _volumes;

        ///////////////////////////////////////////////////////////////
        /// MEMORY BUFFERS DECLARATIONS ///////////////////////////////
        ///////////////////////////////////////////////////////////////

        /* This struct holds buffers and data of the fluid */
        struct FluidData {
            /* Number of particles of the fluid simulation */
            int count;

            cl_mem positions, positions_sorted;
            cl_mem vel_t, vel_t_sorted;
            cl_mem vel_half_t, vel_half_t_sorted;
            cl_mem densities;
            cl_mem pressures;
            cl_mem accelerations;
            cl_mem hashes;
            cl_mem mask;
            cl_mem normals;

            /**
             * This buffer has an entry for every cell on the uniform
             * grid. Every position is an interval [a,b) where,
             * meaning that all particles with indices between 
             * a and b (exclusive) of the sorted particles buffer belong 
             * to the cell. If the cell holds no particles, then a=0 and b=0
             */
            cl_mem cell_intervals;

            /**
             * A buffer that, for every position, has the size of the 
             * neighbourhood list.
             */ 
            cl_mem neigh_list_length;

            /**
             * A buffer that holds, for each particles, a list of indices
             * of the particles in the neighbourhood. It is made of 
             * count * max_neigh_list
             */
            cl_mem neighbourhood_list;
            
            /* Initializes to null all members */
            FluidData();
        } _fluid;

        /*
         * Opengl vbo's for storing particles positions
         * Here we use two buffers to store sorted and unsorted
         * particles on each simulation step
         */ 
        GLuint _vbo_fluid_positions;

        /**
         * Buffers to hold neighbourhood info about static boundary
         */
        cl_mem _sb_neigh_list, _sb_neigh_list_length;

        /**
         * OpenGL-OpenCL shared resources list. We collect here all the 
         * shared buffers references to aquire-release all of them at the same
         * time
         */
        std::vector<cl_mem> _gl_shared_buffers;

        ///////////////////////////////////////////////////////////////
        /// OPENCL KERNEL DECLARATIONS ////////////////////////////////
        ///////////////////////////////////////////////////////////////

        std::unique_ptr<CLProgram> _program;
        cl_kernel _kernel_density_n_pressure;
        cl_kernel _kernel_acceleration;
        cl_kernel _kernel_time_itegration;
        cl_kernel _kernel_normals;

        ///////////////////////////////////////////////////////////////
        /// SIMULATION PARAMETERS /////////////////////////////////////
        ///////////////////////////////////////////////////////////////
      
        // Smoothing kernels constants
        SmoothingConstants _smoothing_constants;
        
        float _dt;
        float _max_vel;
        cl_float4 _g;

        // Fluid physical properties
        float _rest_density;
        float _k_viscosity;
        float _surface_tension;
        float _gas_stiffness;

        // The mass that each partile represents
        float _particle_mass;
        float _particle_radius;

        // The support radius is used to limit the
        // scope of the smoothing kernels
        float _support_radius;
        float _sqr_support_radius;

        // Surface tension model kernel constants
        float _st_kernel_main_constant;
        float _st_kernel_term_constant;

        cl_float4 _container_size;

        // For each particle, this is the maximum particles in the
        // neighbourhood to look at (and save)
        const int _max_neigh_list_length = 100;

        ///////////////////////////////////////////////////////////////
        /// AUXILIARY METHODS /////////////////////////////////////////
        ///////////////////////////////////////////////////////////////

        /**
         * @brief Initializes internal parameters
         * @details Given the global settings, initializes internal parameters
         * like support radius, particle mass, etc.
         * 
         * @param fluid_settings Settings related to the physics of the fluid
         * @param sim_settings Settings related to the simulation method
         */
        void _initialize_params(const PhysicsSettings& fluid_settings,
                                const SimulationSettings& sim_settings);
        /**
         * @brief Initializes the internal device buffers according to the 
         *        fluid particle count
         */
        void _initialize_buffers();

        /**
         * @brief Releses all internal device buffers 
         */
        void _release_buffers();

        /**
         * @brief (Re)Initializes the solver internals
         */
        void _initialize_solver();

        /**
         * @brief Rebuilds the OpenCL program, and reinitializes
         *        all the kernels
         */ 
        void _build_kernels();

        /**
         * @brief Sets up the kernel parameters
         */
        void _setup_kernel_params();
       
        /**
         * @brief Calls a simulation kernel
         * @details This function is used to call internal kernels used by the
         *          solver. The default local_size is 128
         * 
         * @param k [description]
         * @param particle_count [description]
         * @param local_size [description]
         */
        inline void _call_kernel(cl_kernel& k,
                                 int particle_count,
                                 size_t local_size = 128);

        // Generic event to wait for kernels to finish, and to
        // profile kernel times
        cl_event _event;
};

#endif // _WSPH_SIMULATION_H_
