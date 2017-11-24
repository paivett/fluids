#ifndef _CL_COMMON_H_
#define _CL_COMMON_H_

// These structs are used to pass parameters to
// kernels, and avoid passing too many parameters
// that lead to confusion, and errors

typedef struct {
    // The size of the grid
    float size;
    // The size of a cell of the grid. It matches the support radius
    float cell_size;
    // The number of cells per side (its a cube)
    int cells_per_side;
    // The number of cells in the grid: cells_per_side^3
    int cells_count;
} GridInfo;

typedef struct {
    // "Default" kernel constants
    float poly6_eval;
    float poly6_grad;
    float poly6_lapl;
    // Viscosity kernel constants
    float visc_eval;
    float visc_grad;
    float visc_lapl;
    // Spiky kernel constants
    float spiky_eval;
    float spiky_grad;
    float spiky_lapl;

} SmoothingConstants;

#endif // _CL_COMMON_H_