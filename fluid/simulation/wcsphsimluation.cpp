#define CL_USE_DEPRECATED_OPENCL_1_1_APIS   1
#include "wcsphsimluation.h"
#include "opencl/clallocator.h"
#include "opencl/clcompiler.h"
#include "opencl/algorithms/clsort.h"
#include "opencl/algorithms/clshuffle.h"

#include <CL/cl_gl.h>
#include <cmath>

#include <iostream>

using namespace std;

WCSPHSimulation::WCSPHSimulation(const PhysicsSettings& fluid_settings,
                                 const SimulationSettings& sim_settings,
                                 GLuint vbo_fluid_positions) :
_vbo_fluid_positions(vbo_fluid_positions),
_sb_neigh_list(nullptr),
_sb_neigh_list_length(nullptr) {
    // Initialize internal parameters
    _initialize_params(fluid_settings, sim_settings);

    // Initialize internal uniform grid
    cout << "Initializing grid..." << flush;
    _grid = make_unique<Grid>(5.0, _support_radius);
    cout << "done!" << endl;

    // Initialize static boundary handler
    _boundary_handler = make_unique<BoundaryHandler>(*_grid,
                                                     sim_settings.fluid_particle_radius,
                                                     sim_settings.fluid_support_radius,
                                                     _rest_density);
}

int WCSPHSimulation::particle_count() const {
    return _fluid.count;
}

void WCSPHSimulation::simulate() {
    CLAllocator::lock_gl_buffers(_gl_shared_buffers);

    _boundary_handler->sync();

    // First, compute the hash for every particle. The hash depends on the
    // position within the uniform grid
    _grid->compute_hashes(_fluid.positions,
                          _fluid.hashes,
                          _fluid.mask,
                          _fluid.count);

    // Now sort the hashes buffer. Also, sort the mask buffer that will allow
    // to sort the rest of the buffers
    clsort<cl_uint, cl_int>(_fluid.hashes, _fluid.mask, _fluid.count);

    // Now, suffle the positions and velocities
    clshuffle<cl_float4>(_fluid.positions, _fluid.mask, _fluid.positions_sorted,  _fluid.count);
    clshuffle<cl_float4>(_fluid.vel_t, _fluid.mask, _fluid.vel_t_sorted, _fluid.count);
    clshuffle<cl_float4>(_fluid.vel_half_t, _fluid.mask, _fluid.vel_half_t_sorted,  _fluid.count);

    // And compute, for every cell of the uniform grid, the interval within the
    // sorted positions buffer
    _grid->compute_cell_intervals(_fluid.positions_sorted,
                                  _fluid.hashes,
                                  _fluid.cell_intervals,
                                  _fluid.count);

    // And now, compute the neighbourhood list for every fluid particle
    _grid->compute_neigh_list(_fluid.positions_sorted,
                              _fluid.positions_sorted,
                              _fluid.cell_intervals,
                              _fluid.neighbourhood_list,
                              _fluid.neigh_list_length,
                              _max_neigh_list_length,
                              _fluid.count);

    // Also, compute for each fluid particle, the neigbourhood of static 
    // boundary particles
    _boundary_handler->build_neighbourhood(_fluid.positions_sorted,
                                           _sb_neigh_list,
                                           _sb_neigh_list_length,
                                           _max_neigh_list_length,
                                           _fluid.count);

    // Compute initial densities and pressure
    _call_kernel(_kernel_density_n_pressure, _fluid.count);

    // Compute particles normals, used by the surface tension model
    _call_kernel(_kernel_normals, _fluid.count);

    // Compute particles acceleration
    _call_kernel(_kernel_acceleration, _fluid.count);

    // Update rigid bodies
    _boundary_handler->apply_fluid_forces(_fluid.positions_sorted,
                                          _fluid.vel_t_sorted,
                                          _fluid.densities,
                                          _fluid.pressures,
                                          _fluid.cell_intervals,
                                          _particle_mass);

    // Update positions
    _call_kernel(_kernel_time_itegration, _fluid.count);

    CLAllocator::unlock_gl_buffers(_gl_shared_buffers);
}

void WCSPHSimulation::_initialize_buffers() {
    cout << "Initializing buffers..." << flush;

    // Wait to opengl to finish before resizing buffer
    OpenGLFunctions::getFunctions().glFinish();

    _release_buffers();

    // Iterate over all volumes defined to obtain all particles
    vector<cl_float4> fluid_particles;
    for (auto v: _volumes) {
        auto ps = v->particles(_particle_radius);
        fluid_particles.insert(fluid_particles.end(), ps.begin(), ps.end());
    }
    _fluid.count = fluid_particles.size();

    _fluid.positions = CLAllocator::alloc_gl_buffer<cl_float4>(_fluid.count, _vbo_fluid_positions);
    _fluid.positions_sorted = CLAllocator::alloc_buffer<cl_float4>(_fluid.count);

    // Upload the particle positions to the device buffer
    CLAllocator::upload_to_gl_buffer(fluid_particles, _fluid.positions);

    // Initialize buffer for storing the hashes
    _fluid.hashes = CLAllocator::alloc_buffer<cl_uint>(_fluid.count);

    // Initialize buffer mask
    _fluid.mask = CLAllocator::alloc_buffer<cl_int>(_fluid.count);

    // Initialize the buffers that hold the velocity of particles
    cl_float4 zero_float4 = {{0.0f, 0.0f, 0.0f, 0.0f}};
    _fluid.vel_t = CLAllocator::alloc_buffer<cl_float4>(_fluid.count, zero_float4);
    _fluid.vel_t_sorted = CLAllocator::alloc_buffer<cl_float4>(_fluid.count);
    _fluid.vel_half_t = CLAllocator::alloc_buffer<cl_float4>(_fluid.count, zero_float4);
    _fluid.vel_half_t_sorted = CLAllocator::alloc_buffer<cl_float4>(_fluid.count);

    _fluid.densities = CLAllocator::alloc_buffer<cl_float>(_fluid.count);
    _fluid.pressures = CLAllocator::alloc_buffer<cl_float>(_fluid.count);
    _fluid.accelerations = CLAllocator::alloc_buffer<cl_float4>(_fluid.count);
    _fluid.normals = CLAllocator::alloc_buffer<cl_float4>(_fluid.count);

    // Now initialize the buffers related to holding particles neighbourhood
    _fluid.cell_intervals = CLAllocator::alloc_buffer<cl_int2>(_grid->info().cells_count);

    // Initialize the buffer to hold the neighbourhood for each particle
    _fluid.neigh_list_length = CLAllocator::alloc_buffer<cl_int>(_fluid.count);
    _fluid.neighbourhood_list = CLAllocator::alloc_buffer<cl_int>(_fluid.count * _max_neigh_list_length);

    _sb_neigh_list_length = CLAllocator::alloc_buffer<cl_int>(_fluid.count);
    _sb_neigh_list = CLAllocator::alloc_buffer<cl_int>(_fluid.count * _max_neigh_list_length);

    // Reset and store the references to the shared GL buffers
    _gl_shared_buffers.clear();
    _gl_shared_buffers.push_back(_fluid.positions);

    cout << "done!" << endl;
}

void WCSPHSimulation::_initialize_params(const PhysicsSettings& fluid_settings,
                                        const SimulationSettings& sim_settings) {
    cout << "Initializing internal parameters..." << flush;

    _dt = sim_settings.time_step;
    _max_vel = sim_settings.max_vel;

    _rest_density = fluid_settings.rest_density;
    _k_viscosity = fluid_settings.k_viscosity;
    _surface_tension = fluid_settings.surface_tension;
    _gas_stiffness = fluid_settings.gas_stiffness;

    // Global gravity
    _g.s[0] = 0;
    _g.s[1] = -fluid_settings.gravity;
    _g.s[2] = 0;
    _g.s[3] = 0;

    // Estimate the support radius of the kernels
    _particle_radius = sim_settings.fluid_particle_radius;
    _support_radius = sim_settings.fluid_support_radius;
    _sqr_support_radius = _support_radius * _support_radius;

    _particle_mass = _rest_density * pow(2.0f * _particle_radius, 3.0f) / 1.0f;

    // Initialize smoothing kernels constants
    _smoothing_constants.poly6_eval = 315.0 / (64.0 * M_PI * pow(_support_radius, 9));
    _smoothing_constants.poly6_grad = -945.0 / (32.0 * M_PI * pow(_support_radius, 9));
    _smoothing_constants.poly6_lapl =  -945.0 / (32.0 * M_PI * pow(_support_radius, 9));
    _smoothing_constants.spiky_grad = -45.0 / (M_PI * pow(_support_radius, 6));
    _smoothing_constants.visc_eval = 15.0 / (2* M_PI * pow(_support_radius, 3));
    _smoothing_constants.visc_lapl = 45.0 / (M_PI * pow(_support_radius, 6));

    _st_kernel_main_constant = 32.0f / (M_PI * pow(_support_radius, 9.0f));
    _st_kernel_term_constant = pow(_support_radius, 6.0f) / 64.0f;

    cout << "done!" << endl;
}


void WCSPHSimulation::reset(const PhysicsSettings& fluid_settings,
                           const SimulationSettings& sim_settings) {
    // Reset internal params
    _initialize_params(fluid_settings, sim_settings);
    
    _boundary_handler->set_particle_radius(sim_settings.fluid_particle_radius);
    _boundary_handler->set_support_radius(sim_settings.fluid_support_radius);

    _initialize_solver();
}

void WCSPHSimulation::_setup_kernel_params() {
    cout << "Setting up kernel parameters..." << flush;
    if (_fluid.count <= 0) {
        cout << "No fluid particles, skiping kernel params setup" << endl;
        return;
    }

    clSetKernelArg(_kernel_density_n_pressure, 0, sizeof(cl_mem), &_fluid.positions_sorted);   
    clSetKernelArg(_kernel_density_n_pressure, 1, sizeof(cl_mem), &_fluid.densities);
    clSetKernelArg(_kernel_density_n_pressure, 2, sizeof(cl_mem), &_fluid.pressures);
    clSetKernelArg(_kernel_density_n_pressure, 3, sizeof(cl_float), &_particle_mass);
    clSetKernelArg(_kernel_density_n_pressure, 4, sizeof(cl_float), &_support_radius);
    clSetKernelArg(_kernel_density_n_pressure, 5, sizeof(cl_float), &_gas_stiffness);
    clSetKernelArg(_kernel_density_n_pressure, 6, sizeof(cl_float), &_rest_density);
    clSetKernelArg(_kernel_density_n_pressure, 7, sizeof(cl_float), &_smoothing_constants.poly6_eval);
    clSetKernelArg(_kernel_density_n_pressure, 8, sizeof(cl_mem), &_fluid.neigh_list_length);
    clSetKernelArg(_kernel_density_n_pressure, 9, sizeof(cl_mem), &_fluid.neighbourhood_list);
    clSetKernelArg(_kernel_density_n_pressure, 10, sizeof(cl_mem), &_sb_neigh_list);
    clSetKernelArg(_kernel_density_n_pressure, 11, sizeof(cl_mem), &_sb_neigh_list_length);
    clSetKernelArg(_kernel_density_n_pressure, 12, sizeof(cl_mem), _boundary_handler->positions_buffer());
    clSetKernelArg(_kernel_density_n_pressure, 13, sizeof(cl_mem), _boundary_handler->phi_buffer());

    clSetKernelArg(_kernel_acceleration, 0, sizeof(cl_mem), &_fluid.positions_sorted);
    clSetKernelArg(_kernel_acceleration, 1, sizeof(cl_mem), &_fluid.vel_t_sorted);
    clSetKernelArg(_kernel_acceleration, 2, sizeof(cl_mem), &_fluid.normals);
    clSetKernelArg(_kernel_acceleration, 3, sizeof(cl_mem), &_fluid.densities);
    clSetKernelArg(_kernel_acceleration, 4, sizeof(cl_mem), &_fluid.pressures);
    clSetKernelArg(_kernel_acceleration, 5, sizeof(cl_mem), &_fluid.accelerations);
    clSetKernelArg(_kernel_acceleration, 6, sizeof(cl_float4), &_g);
    clSetKernelArg(_kernel_acceleration, 7, sizeof(cl_float), &_particle_mass);
    clSetKernelArg(_kernel_acceleration, 8, sizeof(cl_float), &_support_radius);
    clSetKernelArg(_kernel_acceleration, 9, sizeof(cl_float), &_sqr_support_radius);
    clSetKernelArg(_kernel_acceleration, 10, sizeof(cl_float), &_k_viscosity);
    clSetKernelArg(_kernel_acceleration, 11, sizeof(cl_float), &_surface_tension);
    clSetKernelArg(_kernel_acceleration, 12, sizeof(cl_float), &_rest_density);
    clSetKernelArg(_kernel_acceleration, 13, sizeof(SmoothingConstants), &_smoothing_constants);
    clSetKernelArg(_kernel_acceleration, 14, sizeof(cl_mem), &_fluid.neigh_list_length);
    clSetKernelArg(_kernel_acceleration, 15, sizeof(cl_mem), &_fluid.neighbourhood_list);
    clSetKernelArg(_kernel_acceleration, 16, sizeof(cl_mem), &_sb_neigh_list);
    clSetKernelArg(_kernel_acceleration, 17, sizeof(cl_mem), &_sb_neigh_list_length);
    clSetKernelArg(_kernel_acceleration, 18, sizeof(cl_mem), _boundary_handler->positions_buffer());
    clSetKernelArg(_kernel_acceleration, 19, sizeof(cl_mem), _boundary_handler->phi_buffer());
    clSetKernelArg(_kernel_acceleration, 20, sizeof(cl_float), &_st_kernel_main_constant);
    clSetKernelArg(_kernel_acceleration, 21, sizeof(cl_float), &_st_kernel_term_constant);

    clSetKernelArg(_kernel_normals, 0, sizeof(cl_mem), &_fluid.positions_sorted);
    clSetKernelArg(_kernel_normals, 1, sizeof(cl_mem), &_fluid.densities);
    clSetKernelArg(_kernel_normals, 2, sizeof(cl_mem), &_fluid.normals);
    clSetKernelArg(_kernel_normals, 3, sizeof(cl_mem), &_fluid.neigh_list_length);
    clSetKernelArg(_kernel_normals, 4, sizeof(cl_mem), &_fluid.neighbourhood_list);
    clSetKernelArg(_kernel_normals, 5, sizeof(cl_float), &_smoothing_constants.poly6_grad);
    clSetKernelArg(_kernel_normals, 6, sizeof(cl_float), &_particle_mass);
    clSetKernelArg(_kernel_normals, 7, sizeof(cl_float), &_support_radius);

    clSetKernelArg(_kernel_time_itegration, 0, sizeof(cl_mem), &_fluid.positions_sorted);
    clSetKernelArg(_kernel_time_itegration, 1, sizeof(cl_mem), &_fluid.vel_t_sorted);
    clSetKernelArg(_kernel_time_itegration, 2, sizeof(cl_mem), &_fluid.vel_half_t_sorted);
    clSetKernelArg(_kernel_time_itegration, 3, sizeof(cl_mem), &_fluid.accelerations);
    clSetKernelArg(_kernel_time_itegration, 4, sizeof(cl_float), &_dt);
    clSetKernelArg(_kernel_time_itegration, 5, sizeof(cl_mem), &_fluid.positions);
    clSetKernelArg(_kernel_time_itegration, 6, sizeof(cl_mem), &_fluid.vel_t);
    clSetKernelArg(_kernel_time_itegration, 7, sizeof(cl_mem), &_fluid.vel_half_t);
    clSetKernelArg(_kernel_time_itegration, 8, sizeof(cl_float), &_max_vel);
    clSetKernelArg(_kernel_time_itegration, 9, sizeof(cl_float4), &_container_size);

    cout << "done" << endl;
}

void WCSPHSimulation::_release_buffers() {
    cout << "Releasing buffers..." << flush;
    CLAllocator::release_buffer(_fluid.positions);
    CLAllocator::release_buffer(_fluid.positions_sorted);
    CLAllocator::release_buffer(_fluid.hashes);
    CLAllocator::release_buffer(_fluid.mask);
    CLAllocator::release_buffer(_fluid.vel_t);
    CLAllocator::release_buffer(_fluid.vel_t_sorted);
    CLAllocator::release_buffer(_fluid.vel_half_t);
    CLAllocator::release_buffer(_fluid.vel_half_t_sorted);
    CLAllocator::release_buffer(_fluid.densities);
    CLAllocator::release_buffer(_fluid.pressures);
    CLAllocator::release_buffer(_fluid.accelerations);
    CLAllocator::release_buffer(_fluid.normals);
    CLAllocator::release_buffer(_fluid.cell_intervals);
    CLAllocator::release_buffer(_fluid.neigh_list_length);
    CLAllocator::release_buffer(_fluid.neighbourhood_list);
    CLAllocator::release_buffer(_sb_neigh_list_length);
    CLAllocator::release_buffer(_sb_neigh_list);
    cout << "done" << endl;
}

WCSPHSimulation::~WCSPHSimulation() {
    _release_buffers();
}

void WCSPHSimulation::_call_kernel(cl_kernel& k,
                                  int particle_count,
                                  size_t local_size) {
    size_t global_size = local_size * ceil(particle_count / (float)local_size);

    cl_int err = clEnqueueNDRangeKernel(CLEnvironment::queue(),
                                        k,
                                        1,
                                        NULL,
                                        &global_size,
                                        &local_size,
                                        0,
                                        NULL,
                                        NULL);
    CLError::check(err);
}

WCSPHSimulation::FluidData::FluidData() :
count(0),
positions(nullptr),
positions_sorted(nullptr),
vel_t(nullptr),
vel_t_sorted(nullptr),
vel_half_t(nullptr),
vel_half_t_sorted(nullptr),
densities(nullptr),
pressures(nullptr),
accelerations(nullptr),
hashes(nullptr),
mask(nullptr),
normals(nullptr),
cell_intervals(nullptr),
neigh_list_length(nullptr),
neighbourhood_list(nullptr)
{}

void WCSPHSimulation::add_boundary(const shared_ptr<RigidBody> boundary,
                                   const string& boundary_id,
                                   bool can_move) {
    _boundary_handler->add_boundary(boundary, 
                                    boundary_id, 
                                    can_move);

    // We reset all kernel params so that the static boundary buffers are
    // updated
    _build_kernels();
}

void WCSPHSimulation::_build_kernels() {
    // First, the program must be compiled
    CLCompiler compiler;
    compiler.add_file_source("kernels/wcsph.cl");
    compiler.add_file_source("kernels/normals.cl");
    compiler.add_build_option("-cl-std=CL1.2");
    compiler.add_build_option("-cl-fast-relaxed-math");
    compiler.add_include_path("kernels");

    // Define the constants for the program
    compiler.define_constant("PARTICLE_COUNT", _fluid.count);
    compiler.define_constant("SUPPORT_RADIUS", _support_radius);
    compiler.define_constant("USE_MULLER_KERNELS");
    _program = compiler.build();

    // Now reinitialize all kernel instances
    _kernel_density_n_pressure = _program->get_native_kernel("compute_density_pressure");
    _kernel_acceleration = _program->get_native_kernel("compute_acceleration");
    _kernel_normals = _program->get_native_kernel("compute_normals");
    _kernel_time_itegration = _program->get_native_kernel("predict_vel_n_pos");

    _setup_kernel_params();
}

int WCSPHSimulation::boundary_particle_count() const {
    return _boundary_handler->particle_count();
}

void WCSPHSimulation::add_volume(const std::shared_ptr<FluidVolume> volume) {
    _volumes.push_back(volume);

    // Reinitialize the whole solver to hold the new particles
    _initialize_solver();
}

void WCSPHSimulation::_initialize_solver() {
    // Reset grid cell size with the new support radius
    _grid->set_cell_size(_support_radius);

    // Reinitialize all buffers
    _initialize_buffers();

    // Rebuild all kernels
    _build_kernels();

    // Configure kernel parameters once, call them later many times
    _setup_kernel_params();
}

void WCSPHSimulation::set_rect_limits(float width, float height, float depth) {
    _container_size.s[0] = width;
    _container_size.s[1] = height;
    _container_size.s[2] = depth;
    _container_size.s[3] = 0.0f;

    _setup_kernel_params();
}