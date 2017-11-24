#ifndef _CL_GRID_H_
#define _CL_GRID_H_

#include "common.h"
#include "morton.h"

#define CELL_ID(x, y, z, grid_info) (mad24(grid_info.cells_per_side, mad24(grid_info.cells_per_side, z, y), x))

#ifdef USE_MORTON_ENCODING
    #define CELL_HASH(x, y, z, grid_info)   (morton_3d_encode(x, y, z))
#else
    #define CELL_HASH(x, y, z, grid_info)   (CELL_ID(x, y, z, grid_info))
#endif

constant int4 CELL_NEIGH_OFFSET[27] = {
    // the first cell to look for particles 
    // is the same cell of the particle !
    (int4)(0,  0,  0, 0), 

    (int4)(-1, -1, -1, 0),
    (int4)(-1, -1,  0, 0), 
    (int4)(-1, -1,  1, 0),
    (int4)(-1,  0, -1, 0),
    (int4)(-1,  0,  0, 0), 
    (int4)(-1,  0,  1, 0),
    (int4)(-1,  1, -1, 0), 
    (int4)(-1,  1,  0, 0),
    (int4)(-1,  1,  1, 0),
    
    (int4)(0, -1, -1, 0),
    (int4)(0, -1,  0, 0), 
    (int4)(0, -1,  1, 0),
    (int4)(0,  0, -1, 0), 
    
    (int4)(0,  0,  1, 0), 
    (int4)(0,  1, -1, 0),
    (int4)(0,  1,  0, 0),
    (int4)(0,  1,  1, 0), 
    
    (int4)(1, -1, -1, 0), 
    (int4)(1, -1,  0, 0),
    (int4)(1, -1,  1, 0), 
    (int4)(1,  0, -1, 0),
    (int4)(1,  0,  0, 0), 
    (int4)(1,  0,  1, 0),
    (int4)(1,  1, -1, 0), 
    (int4)(1,  1,  0, 0),
    (int4)(1,  1,  1, 0)
};



/**
 * @brief Computes the coordinates within the 3d-grid.
 * @details Given a position in space (inside the uniform grid) and the grid 
 *          sizes this function computes the cell id as (x,y,z) where each 
 *          component tells the position of the cell in the grid.
 *
 * @param p The position to evaluate.
 * @param grid_info The information of the uniform grid.
 * 
 * @return The coordinate within the grid.
 */
inline int4 get_grid_coordinates(const float4* p, const GridInfo* grid_info) {
    int4 c;
    // Particles reference system is in the middle of the grid,
    // and the grid is a cube, thats why the "+ grid_size/2"
    c.x = floor((p->x + grid_info->size/2.0f) / grid_info->cell_size);
    c.y = floor((p->y + grid_info->size/2.0f) / grid_info->cell_size);
    c.z = floor((p->z + grid_info->size/2.0f) / grid_info->cell_size);

    c.x = c.x + grid_info->cells_per_side;
    c.y = c.y + grid_info->cells_per_side;
    c.z = c.z + grid_info->cells_per_side;

    c.x = c.x % grid_info->cells_per_side;
    c.y = c.y % grid_info->cells_per_side;
    c.z = c.z % grid_info->cells_per_side;

    return c;
}

/**
 * @brief Returns the cell id of a position within the uniform grid.
 * @details Given a position inside the uniform grid, and the grid dimensions 
 * and properties, this function translates the 3d cell id of the position, to 
 * a planar (1-d) index.
 *
 * @param p The position to evaluate.
 * @param grid_info The information of the uniform grid.
 * 
 * @return The cell id.
 */
inline int compute_cell_id(const float4* p,
                           const GridInfo* grid_info) {
    int4 cell = get_grid_coordinates(p, grid_info);
    return CELL_ID(cell.x, cell.y, cell.z, (*grid_info));
} 

#endif // _CL_GRID_H_