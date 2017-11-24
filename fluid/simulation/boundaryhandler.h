#ifndef _BOUNDARY_HANDLER_H_ 
#define _BOUNDARY_HANDLER_H_ 

#include "grid.h"
#include "scene/rigidbody.h"
#include "opencl/clcompiler.h"
#include <LinearMath/btVector3.h>
#include <CL/cl.h>
#include <vector>

/**
 * @class BoundaryHandler
 * @brief Handles the boundaries of rigid bodies
 * 
 * @details This class manages the host and device memory of particles that 
 * represent rigid body boundaries. Provides the necessary data for the fluid 
 * simulation to include boundaries in the simulation.
 */
class BoundaryHandler {
    public:
        /**
         * @brief Constructs a new BoudaryHandler
         * 
         * @param grid The uniform grid used on the fluid simulation.
         * @param particle_spacing The spacing between particles.
         * @param support_radius The support radius to use.
         * @param rest_density The rest density of the fluid the boundary
         *                     interacts with.
         * @param phi_coefficient Boundary phi computation is divided by this 
         *                        coefficient.
         */
        BoundaryHandler(const Grid& grid,
                        float particle_spacing,
                        float support_radius,
                        float rest_density,
                        float phi_coefficient=1.0);

        /* Destructor */
        ~BoundaryHandler();

        /**
         * @brief Adds a new sampling of a static boundary surface
         * 
         * @param boundary_sampling A vector of particles that sample the 
         *                          boundary surface
         * @param boundary_id A unique identifier for the boundary
         */
        void add_boundary(std::shared_ptr<RigidBody> boundary, 
                          const std::string& boundary_id,
                          bool can_move=false);

        /**
         * @brief Returns the total number of particles that belong to the 
         *        boundary
         * @return The static boundary particle count
         */
        int particle_count() const;

        /**
         * @brief Synchronizes with the rigid bodies
         * @details Synchronizes the buffers with positions of the boundary 
         *          samping particles with the current position and rotation of 
         *          the rigid bodies, only if the body has been tagged as 
         *          movable.
         * @param sync_all syncs all particles, regardless if the body can or
         *        cannot move.
         */     
        void sync(bool sync_all=false);

        /**
         * @brief Updates the particle radius of the boundaries. This will 
         *        trigger a resampling of all surfaces and internal buffers.
         * 
         * @param particle_radius The particle radius of the boundaries 
         *                        sampling.
         */
        void set_particle_radius(float particle_radius);

        /**
         * @brief Returns the internal particle radius.
         * @return The configured sampling particle radius.
         */
        float particle_radius() const;

        /**
         * @brief Updates the support radius of the boundary handler.
         * 
         * @details Updates the support radius of the boundary handler. This
         *          will trigger a resampling of all surfaces and internal 
         *          buffers. This is the internal support radius, used to 
         *          compute boundary phi values, and neigh lists of 
         *          boundary particles. This can be different to the fluid 
         *          solver support radius.
         * 
         * @param particle_radius The particle radius of the boundaries 
         *                        sampling.
         */
        void set_support_radius(float support_radius);

        /**
         * @brief Returns the internal support radius.
         * @return The configured sampling support radius.
         */
        float support_radius() const;

        /**
         * @brief Updates the phi coefficient
         *
         * @param phi_coeff Boundary phi is divided by this coefficient
         */
        void set_phi_coefficient(float phi_coeff);

        /**
         * @brief Builds the neigh list for each reference particle.
         * @details Given a buffer of particles (the reference particles, 
         *          probably fluid particles) computes, for each reference 
         *          particle, its neighbourhood list of boundary particles.
         *          It uses the local grid for particle lookup so check that 
         *          the fluid implementation is using the same grid instance.
         * 
         * @param ref_positions The buffer of reference positions.
         * @param neigh_list A buffer to save the list for each ref particle.
         * @param neigh_list_length A buffer to save the length of each list.
         * @param max_list_length The max length of each list.
         * @param particle_count The number of ref particles.
         */
        void build_neighbourhood(cl_mem ref_positions,
                                 cl_mem neigh_list,
                                 cl_mem neigh_list_length,
                                 int max_list_length,
                                 int particle_count) const;

        void apply_fluid_forces(cl_mem fluid_particles,
                                cl_mem fluid_velocities,
                                cl_mem fluid_densities,
                                cl_mem fluid_pressures,
                                cl_mem fluid_cell_intervals,
                                float particle_mass);

        /**
         * @brief A pointer to the positions of boundary particles
         */
        cl_mem* positions_buffer();

        cl_mem* velocities_buffer();

        //cl_mem* cell_intervals_buffer();

        cl_mem* phi_buffer();

        // cl_mem* fluid_force_buffer();

    private:
        // The total count of particles that belong to the boundary surface
        int _count;

        float _particle_spacing;
        float _support_radius;
        float _poly6_eval;
        float _rest_density;
        float _phi_coefficient;

        // A reference to the world uniform grid
        const Grid& _grid;

        struct _SurfaceInfo {
            cl_mem raw_sub_buffer;
            cl_mem transformed_sub_buffer;
            cl_mem phi_sub_buffer;
            cl_mem vel_sub_buffer;
            cl_mem forces;
            cl_mem torque;
            std::shared_ptr<RigidBody> body;
            int particle_count;
            // Not in bytes, but in number of elements
            int buff_origin;
            int buff_size;
            // If this flag is false, then the sync method will 
            // avoid synchronization  of particles for this surface
            bool can_move;
        };

        // A dict that tells the start, and end positions within the 
        // raw positions buffer of every rigid body
        std::vector<_SurfaceInfo> _bodies;

        // The original position of every boundary particle, unsorted. All 
        // particles of the same body are together.
        cl_mem _raw_positions;
        // The unsorted positions of every boundary particle
        cl_mem _unsorted_positions;
        // The position of every boundary particle, sorted by the grid hashes
        cl_mem _sorted_positions;
        // Unsorted and sorted velocities buffers
        cl_mem _velocities, _sorted_velocities;
        // For each cell in the uniform grid, an interval within the _positions
        // buffer that indicates which particles belong to the cell
        cl_mem _cell_intervals;
        // Phi density estimation for each boundary particle
        cl_mem _phi;
        // Phi density estimation sorted
        cl_mem _sorted_phi;
        // The force being excerted by the fluid for each boundary particle
        cl_mem _fluid_force;

        // Temporal buffers for sorting
        cl_mem _hashes, _tmp_hashes;
        cl_mem _mask, _tmp_mask;

        std::unique_ptr<CLProgram> _program;
        std::shared_ptr<CLKernel> _kernel_boundary_phi;
        std::shared_ptr<CLKernel> _kernel_fluid_force;
        std::shared_ptr<CLKernel> _kernel_transform_particles;

        // This buffer holds all particles of boundaries together
        std::vector<cl_float4> _particles;

        /**
         * @brief [brief description]
         * @details [long description]
         */
        void _sample_surfaces();

        /**
         * @brief Releases all internal buffers
         */
        void _release();

        /**
         * @brief Allocates new buffers for boundary data
         */
        void _alloc_buffers();

        /**
         * @brief Sorts positions buffer according to the grid hashes
         */
        void _sort_positions();

        /**
         * @brief Computes the phi factor for each boudnary particle. This 
         * value is used in density estimation for particles close to the 
         * boundary.
         */
        void _compute_boundary_phi();

        /**
         * @brief Builds (compiles and link) the OpenCL program, reinitializes
         *        all the internal kernels, and sets the parameters.
         */
        void _build_kernels();

        /**
         * @brief Clears the internal state
         */
        void _clear();
};

#endif // _STATIC_BOUNDARY_H_ 