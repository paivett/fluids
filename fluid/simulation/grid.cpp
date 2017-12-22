#include "grid.h"
#include "opencl/clallocator.h"
#include "opencl/clmisc.h"
#include <vector>

using namespace std;

Grid::Grid(float size, float cell_size) {
    // Initialize grid info struct
    _reset(size, cell_size);
    _sqr_support_radius = cell_size * cell_size;
}

Grid::~Grid() {

}

int Grid::cell_count() const {
    return _grid_info.cells_count;
}

void Grid::compute_hashes(cl_mem positions,
                          cl_mem hashes,
                          cl_mem mask,
                          int count) const {
    _kernel_hashes->set_arg(0, &positions);
    _kernel_hashes->set_arg(1, &hashes);
    _kernel_hashes->set_arg(2, &mask);
    _kernel_hashes->set_arg(3, &_grid_info);
    _kernel_hashes->set_arg(4, &count);

    auto err = _kernel_hashes->run(count, 32);
    CLError::check(err);
}

void Grid::compute_cell_intervals(cl_mem positions,
                                  cl_mem hashes,
                                  cl_mem intervals,
                                  int count) const {
    // First, clean the intervals buffer with zeros
    auto err = CLAllocator::fill_buffer(intervals, _zero_int2, _grid_info.cells_count);
    CLError::check(err);

    _kernel_intervals->set_arg(0, &positions);
    _kernel_intervals->set_arg(1, &hashes);
    _kernel_intervals->set_arg(2, &intervals);
    _kernel_intervals->set_arg(5, &_grid_info);
    _kernel_intervals->set_arg(6, &count);
    _kernel_intervals->set_local_buffer(3, sizeof(cl_float4));
    _kernel_intervals->set_local_buffer(4, sizeof(cl_uint));

    // Now call the kernel
    err = _kernel_intervals->run(count, 32);
    CLError::check(err);
}

GridInfo Grid::info() const {
    return _grid_info;
}

void Grid::_reset(float size, float cell_size) {
    _grid_info.size = size;
    _grid_info.cell_size = cell_size;
    _grid_info.cells_per_side = ceil(_grid_info.size / _grid_info.cell_size);
    _grid_info.cells_count = pow(_grid_info.cells_per_side, 3.0);

    _build_kernels();
}

void Grid::set_size(float size) {
    _reset(size, _grid_info.cell_size);
}

void Grid::set_cell_size(float cell_size) {
    _reset(_grid_info.size, cell_size);
}

void Grid::_build_kernels() {
    CLCompiler compiler;
    compiler.add_file_source("kernels/grid.cl");
    compiler.add_build_option("-cl-std=CL1.2");
    compiler.add_build_option("-cl-fast-relaxed-math");
    compiler.add_build_option("-cl-mad-enable");
    compiler.add_include_path("kernels");
    compiler.define_constant("USE_MORTON_ENCODING");
    compiler.define_constant("CELL_SIZE", _grid_info.cell_size);
    compiler.define_constant("GRID_SIZE", _grid_info.size);
    compiler.define_constant("CELLS_PER_SIDE", _grid_info.cells_per_side);

    _program = compiler.build();
  
    _kernel_hashes = _program->get_kernel("compute_hashes");
    _kernel_intervals = _program->get_kernel("compute_cell_intervals");
    _kernel_neigh_list = _program->get_kernel("compute_neigh_list");
}

void Grid::compute_neigh_list(cl_mem ref_positions,
                              cl_mem neigh_positions,
                              cl_mem intervals,
                              cl_mem neigh_list,
                              cl_mem neigh_list_length,
                              int max_list_length,
                              int count) const {
    _kernel_neigh_list->set_arg(0, &ref_positions);
    _kernel_neigh_list->set_arg(1, &neigh_positions);
    _kernel_neigh_list->set_arg(2, &intervals);
    _kernel_neigh_list->set_arg(3, &neigh_list);
    _kernel_neigh_list->set_arg(4, &neigh_list_length);
    _kernel_neigh_list->set_arg(5, &_grid_info);
    _kernel_neigh_list->set_arg(6, &count);

    // Now call the kernel
    auto err = _kernel_neigh_list->run(count);
    CLError::check(err);
}