/** 
 *  @file wsphsimulation.h
 *  @brief Contains the declaration of the WSPHSimulation class.
 *
 *  This contains the prototype for the WSPHSimulation class. This solver 
 *  implements the Weakly Compressible SPH.
 *  
 *  @author Santiago Daniel Pivetta
 */

#ifndef _UNIFORM_GRID_H_ 
#define _UNIFORM_GRID_H_

#include "opencl/clenvironment.h"
#include "opencl/clcompiler.h"
#include "kernels/common.h"
#include <memory>

/**
 * @class Grid
 * @brief The uniform grid
 * @details This is a uniform grid. A cube divided into little cells, each a 
 *          little cube. The size of the cells should match the size of the 
 *          smoothing radius to provide an accurate and efficient neighbourhood 
 *          search.
 */
class Grid {

    public:
        /**
         * @brief Constructor
         * @details Creates a new uniform grid. The grid is a cube with a side
         * of size length. Every cell of the uniform grid is a cube of cell_size 
         * side.
         * 
         * @param size The size of the uniform cubic grid.
         * @param cell_size The side of the cell. The cell is also a cube
         */
        Grid(float size, float cell_size);

        /**
         * @brief Destructor
         * @details Releases all resources allocated by the Grid instance
         */
        ~Grid();

        /**
         * @brief Returns the number of cells of the grid
         * @return The number of cells
         */
        int cell_count() const;

        /**
         * @brief Computes hashes of every position
         * @details Given a list of positions within the uniform grid, computes 
         *          a list of hashes for each particle. It also initializes a 
         *          mask buffer with the identity.
         * 
         * @param positions A device buffer of cl_float4 holding the positions 
         *                  of every particle
         * @param hashes A device buffer of cl_uint that will hold the hashes
         * @param mask A device buffer of cl_int that will be initialized with 
         *             the identity buffer (eg: mask[i] = i)
         * @param count The number of particles
         */
        void compute_hashes(cl_mem positions,
                            cl_mem hashes,
                            cl_mem mask,
                            int count) const;

        /**
         * @brief For every cell, computes the interval within positions buffer
         * @details Given a list of positions, and a list of hashes, one for 
         *          each position, computes, for every grid's cell, an interval 
         *          of indices of the positions buffer. That interval means 
         *          that all the particles that lie on that cell, are located 
         *          within that interval in the positions buffer.
         *          The hashes buffer must be sorted, and for every position i, 
         *          the hash of that position also is the "i".
         * 
         * @param positions A device buffer of cl_float4 holding the sorted
         *                  positions of every particle
         * @param hashes A device buffer of cl_uint that holds the sorted hashes
         * @param intervals A device buffer of cl_int2 that will hold for each
         *                  grid cell the interval of particles that lie within
         * @param count The number of particles
         */
        void compute_cell_intervals(cl_mem positions,
                                    cl_mem hashes,
                                    cl_mem intervals,
                                    int count) const;

        /**
         * @brief For each particle, generates a neighbourhood list
         * @details For every particle in the reference 
         *          buffer, a neighbourhood list. This list is made 
         *          from particles in the neigh_positions buffer.
         * 
         * @param ref_positions Buffer of reference positions.
         * @param neigh_positions Buffer of sorted particles to 
         *                        build the neigh list from.
         * @param intervals A device buffer of cl_int2 that holds for each
         *                  grid cell the interval of particles that lie within.
         *                  The particles are the ones in the neigh_positions 
         *                  buffer.
         * @param neigh_list A buffer to save the neigh list for every ref particle.
         * @param neigh_list_length A buffer to save the neigh list length for 
         *                          every ref particle.
         * @param max_list_length The max length of each neigh list.
         * @param count The number of ref particles.
         */
        void compute_neigh_list(cl_mem ref_positions,
                                cl_mem neigh_positions,
                                cl_mem intervals,
                                cl_mem neigh_list,
                                cl_mem neigh_list_length,
                                int max_list_length,
                                int count) const;

        /**
         * @brief Returns grid info
         * @details Returns a copy of a grid info struct, that has details
         *          about the uniform grid.
         * @return An instance of GridInfo struct
         */
        GridInfo info() const;

        /**
         * @brief Sets a new size for the grid
         * 
         * @param size Grid size
         */
        void set_size(float size);

        /**
         * @brief Sets a new cell size for the uniform grid cells
         * 
         * @param cell_size The size of the cubic cell
         */
        void set_cell_size(float cell_size);

    private:
        std::unique_ptr<CLProgram> _program;

        std::shared_ptr<CLKernel> _kernel_hashes;
        std::shared_ptr<CLKernel> _kernel_intervals;
        std::shared_ptr<CLKernel> _kernel_neigh_list;

        GridInfo _grid_info;

        // Auxiliary
        const cl_int2 _zero_int2 = {{0, 0}};

        float _sqr_support_radius;

        void _reset(float size, float cell_size);

        void _compute_adj_map();

        void _build_kernels();
};

#endif // _UNIFORM_GRID_H_
