/** 
 *  @file pcisphsimulation.h
 *  @brief Contains the declaration of the PCISPHSimulation class.
 *
 *  This contains the prototype for the PCISPHSimulation class. This solver 
 *  implements the Predictive Compresible incompressible SPH.
 *  
 *  @author Santiago Daniel Pivetta
 */

#ifndef _PCIWSPH_SIMULATION_H_
#define _PCIWSPH_SIMULATION_H_

#include <vector>
#include <memory>

#include "opencl/clenvironment.h"
#include "opencl/clcompiler.h"
#include "opengl/openglfunctions.h"
#include "fluidsimulation.h"
#include "grid.h"
#include "boundaryhandler.h"
#include "kernels/common.h"
#include "mullerconstants.h"

/**
 * @class PCISPHSimulation
 * @brief Implementation based on PCI-SPH
 * @details This implementation is based on a PCISPH solver. For further details
 *          - "Predictive-Corrective Incompressible SPH"
 * 
 */
class PCISPHSimulation : public FluidSimulation {
    public:
        PCISPHSimulation(const PhysicsSettings& fluid_settings,
                         const SimulationSettings& sim_settings,
                         GLuint vbo_particles);

        ~PCISPHSimulation();

        /**
         * @brief Simulates a delta-t step of the pci-sph solver
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
        // The number of particles of the simulation
        int _particle_count;

        // Uniform grid
        std::unique_ptr<Grid> _grid;

        // Boundary handler
        std::unique_ptr<BoundaryHandler> _boundary_handler;

        // A collection of fluid volumes
        std::vector<std::shared_ptr<FluidVolume> > _volumes;

        ///////////////////////////////////////////////////////////////
        /// MEMORY BUFFERS DECLARATIONS ///////////////////////////////
        ///////////////////////////////////////////////////////////////

        // Opengl vbo's for storing particles positions
        // Here we use two buffers to store sorted and unsorted
        // particles on each simulation step
        GLuint _vbo_positions;

        // OpenGL-OpenCL shared resources list. We collect here all the 
        // shared buffers references to aquire-release all of them at the same
        // time
        std::vector<cl_mem> _gl_shared_buffers;

        // These two buffers are the memory buffers to store particles
        // positions. The memory is shared with the opengl VBO's
        cl_mem _positions_unsorted;
        cl_mem _positions_sorted;
        cl_mem _positions_predicted;

        // This buffer is used to store the particles hashes. Hashes
        // are computed for each particle using the particle's position.
        // Then the position buffer is sorted according to this buffer
        cl_mem _hashes;

        // Particle mask holds, for each buffer position, which element
        // of the unsorted buffer should be. It is used to reorder the 
        // buffers once the hashes had been sorted.
        cl_mem _mask;

        // Buffers that store the velocities of the particles.
        cl_mem _velocities_unsorted;
        cl_mem _velocities_sorted;
        // This buffer is used to hold the predicted velocities when performing
        // the iterations of pcisph
        cl_mem _velocities_predicted;
        
        // Buffer to store, for each particle, its computed density.
        // On each iteration, density is recomputed for each 
        // particle, so there is no need for extra buffers or reordering
        cl_mem _mass_densities;
        // A new buffer to hold the predicted densities on the pcisph loop
        cl_mem _mass_densities_predicted;
        cl_mem _mass_density_variation;

        // Buffer to store, for each particle, its computed pressure.
        // On each iteration, pressure is recomputed for each 
        // particle, so there is no need for extra buffers or reordering
        cl_mem _pressures;

        // Buffer to store the acting forces on each particle: viscosity, 
        // surface tension, wieght, and any external force
        cl_mem _particle_force;

        // Buffer to store the particles pressure force
        cl_mem _pressure_force;

        // Particles normals used for the surface tension model
        cl_mem _normals;

        // This buffer has an entry for every cell on the uniform
        // grid. Every position is an interval [a,b) where,
        // meaning that all particles with indices between 
        // a and b (exclusive) of the sorted particles buffer belong 
        // to the cell. If the cell holds no particles, then a=0 and b=0
        cl_mem _cell_intervals;

        // A buffer that, for every position, has the size of the 
        // neighbourhood list.
        cl_mem _neigh_list_length;

        // A buffer that holds, for each particles, a list of indices
        // of the particles in the neighbourhood. It is made of 
        // #particles * max_neigh_list
        cl_mem _neighbourhood_list;

        // Neigh list for static boundaries
        cl_mem _sb_neigh_list;
        cl_mem _sb_neigh_list_length;
     
        ///////////////////////////////////////////////////////////////
        /// OPENCL KERNEL DECLARATIONS ////////////////////////////////
        ///////////////////////////////////////////////////////////////

        std::unique_ptr<CLProgram> _program;
        std::shared_ptr<CLKernel> _kernel_initial_density;
        std::shared_ptr<CLKernel> _kernel_initial_forces;
        std::shared_ptr<CLKernel> _kernel_initial_st;
        std::shared_ptr<CLKernel> _kernel_normals;
        std::shared_ptr<CLKernel> _kernel_predict_vel_n_pos;
        std::shared_ptr<CLKernel> _kernel_update_pressure;
        std::shared_ptr<CLKernel> _kernel_compute_pressure_force;
        std::shared_ptr<CLKernel> _kernel_compute_boundary_phi;

        ///////////////////////////////////////////////////////////////
        /// SIMULATION PARAMETERS /////////////////////////////////////
        ///////////////////////////////////////////////////////////////
      
        // Smoothing constants
        MullerConstants _smoothing_constants;
        
        float _dt;
        float _max_vel;
        cl_float4 _g;

        // Fluid physical properties
        float _rest_density;
        float _k_viscosity;
        float _surface_tension;

        // The mass that each partile represents
        float _particle_mass;

        // The support radius is used to limit the
        // scope of the smoothing kernels
        float _particle_radius;
        float _support_radius;
        float _sqr_support_radius;
        float _density_scale_factor;

        cl_float4 _container_size;
        
        // Surface tension model kernel constants
        float _st_kernel_main_constant;
        float _st_kernel_term_constant;

        // Local size to use by opencl kernels
        size_t _kernel_local_size;

        // For each particle, this is the maximum particles in the
        // neighbourhood to look at (and save)
        const int _max_neigh_list_length = 50;

        // 
        const int _min_iterations = 3;
        const int _max_iterations = 7;
        const float _density_variation_threshold = 0.1;

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
         * @brief (Re)Initializes the solver internals
         */
        void _initialize_solver();

        /**
         * @brief Initializes the internal device buffers according to the 
         * fluid particle count
         */
        void _initialize_buffers();

        /**
         * @brief Rebuilds the OpenCL program, and reinitializes
         *        all the kernels
         */ 
        void _build_kernels();

        /**
         * @brief [brief description]
         * @details [long description]
         * 
         * @param min_iter [description]
         * @param max_iter [description]
         */
        void _simulate_pcisph_step(int min_iter, int max_iter);

        // Auxiliary methods
        void _release_buffers();
        
        void _setup_kernel_params();
        void _deduce_density_scale_factor();

        float _get_max_density_variation();
        
        // Generic event to wait for kernels to finish, and to
        // profile kernel times
        cl_event _event;
};

#endif // _PCIWSPH_SIMULATION_H_
