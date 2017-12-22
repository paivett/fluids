#define CL_USE_DEPRECATED_OPENCL_1_1_APIS   1

#include "pcisphsimluation.h"
#include "fluidvolume.h"
#include "opencl/clallocator.h"
#include "opencl/algorithms/clsort.h"
#include "opencl/algorithms/clshuffle.h"
#include "opencl/algorithms/clreduce.h"

#include <CL/cl_gl.h>
#include <iostream>

using namespace std;


const cl_float4 CL_FLOAT4_ZERO = {{0, 0, 0, 0}};


PCISPHSimulation::PCISPHSimulation(const PhysicsSettings& fluid_settings,
                                   const SimulationSettings& sim_settings,
                                   GLuint vbo_positions) :
_particle_count(0),
_vbo_positions(vbo_positions),
_positions_unsorted(nullptr),
_positions_sorted(nullptr),
_positions_predicted(nullptr),
_hashes(nullptr),
_mask(nullptr),
_velocities_unsorted(nullptr),
_velocities_sorted(nullptr),
_velocities_predicted(nullptr),
_mass_densities(nullptr),
_mass_densities_predicted(nullptr),
_mass_density_variation(nullptr),
_pressures(nullptr),
_particle_force(nullptr),
_pressure_force(nullptr),
_normals(nullptr),
_cell_intervals(nullptr),
_neigh_list_length(nullptr),
_neighbourhood_list(nullptr),
_sb_neigh_list(nullptr),
_sb_neigh_list_length(nullptr),
_image_positions(nullptr),
_image_predicted_positions(nullptr),
_image_velocities(nullptr),
_image_densities(nullptr),
_image_normals(nullptr),
_image_pressures(nullptr)
{
    // Initialize internal parameters
    _initialize_params(fluid_settings, sim_settings);
    // Initialize internal uniform grid
    _grid = make_unique<Grid>(5.0, _support_radius);

    // Initialize static boundary handler
    _boundary_handler = make_unique<BoundaryHandler>(*_grid,
                                                     sim_settings.fluid_particle_radius,
                                                     sim_settings.fluid_support_radius,
                                                     _rest_density,
                                                     1.0f);
}

int PCISPHSimulation::particle_count() const {
    return _particle_count;
}

void PCISPHSimulation::simulate() {   
    _simulate_pcisph_step(_min_iterations, _max_iterations);
}

void PCISPHSimulation::_simulate_pcisph_step(int min_iter, int max_iter) {
    CLAllocator::lock_gl_buffers(_gl_shared_buffers);

    _boundary_handler->sync();

    // First, compute the hash for every particle. The hash depends on the 
    // position within the uniform grid
    _grid->compute_hashes(_positions_unsorted, 
                          _hashes, 
                          _mask, 
                          _particle_count);

    // Now sort the hashes buffer. Also, sort the mask buffer that will allow
    // to sort the rest of the buffers
    clsort<cl_uint, cl_int>(_hashes, _mask, _particle_count);

    // Now, suffle the positions and velocities
    clshuffle<cl_float4>(_positions_unsorted, _mask, _positions_sorted, _particle_count);
    clshuffle<cl_float4>(_velocities_unsorted, _mask, _velocities_sorted, _particle_count);

    // And compute, for every cell of the uniform grid, the interval within 
    // the sorted positions buffer
    _grid->compute_cell_intervals(_positions_sorted, 
                                  _hashes,
                                  _cell_intervals, 
                                  _particle_count);

    // And now, compute the neighbourhood list for every fluid particle
    _grid->compute_neigh_list(_positions_sorted,
                              //_positions_sorted,
                              _image_positions,
                              _cell_intervals,
                              _neighbourhood_list,
                              _neigh_list_length,
                              _max_neigh_list_length,
                              _particle_count);

    // Also, compute for each fluid particle, the neigbourhood of static 
    // boundary particles
    _boundary_handler->build_neighbourhood(_positions_sorted,
                                           _sb_neigh_list,
                                           _sb_neigh_list_length,
                                           _max_neigh_list_length,
                                           _particle_count);
    
    // Compute initial densities
    CLAllocator::fill_buffer(_pressures, 0.0f, _particle_count);
    _kernel_initial_density->run(_particle_count);

    // Compute particles normals for the surface tension model
    _kernel_normals->run(_particle_count);

    // Compute the initial forces: viscosity, surface tension, and
    // rigid body interaction
    CLAllocator::fill_buffer(_pressure_force, CL_FLOAT4_ZERO, _particle_count);
    _kernel_initial_forces->run(_particle_count);

    for (int i=1; i <= max_iter; ++i) {
        // Predict the positions and velocities of the particles to
        // temporal buffers with the current forces
        _kernel_predict_pos->set_arg(4, &_positions_predicted);
        _kernel_predict_pos->run(_particle_count);

        // Update the particles pressures according to the new
        // predicted positions
        _kernel_update_pressure->run(_particle_count);

        // With the updated pressures, now compute the pressure force      
        _kernel_compute_pressure_force->run(_particle_count);

        if(i >= min_iter) {
            float max_density_variation = _get_max_density_variation();
            if((max_density_variation-_rest_density)/_rest_density < _density_variation_threshold) {
                break;
            }
        }
    }

    // Update rigid bodies
    _boundary_handler->apply_fluid_forces(_image_positions,
                                          _image_velocities,
                                          _image_densities,
                                          _image_pressures,
                                          _cell_intervals,
                                          _particle_mass);

    // Predict the velocities and positions with the final 
    // pressure force
    _kernel_predict_vel_n_pos->set_arg(4, &_positions_unsorted);
    _kernel_predict_vel_n_pos->set_arg(5, &_velocities_unsorted);
    _kernel_predict_vel_n_pos->run(_particle_count);

    CLAllocator::unlock_gl_buffers(_gl_shared_buffers);
}

void PCISPHSimulation::_initialize_buffers() {
    // Wait to opengl to finish before resizing buffer
    OpenGLFunctions::getFunctions().glFinish();

    _release_buffers();

    // Iterate over all volumes defined to obtain all particles
    vector<cl_float4> fluid_particles;
    for (auto v: _volumes) {
        auto ps = v->particles(_particle_radius);
        fluid_particles.insert(fluid_particles.end(), ps.begin(), ps.end());
    }
    _particle_count = fluid_particles.size();

    _positions_unsorted = CLAllocator::alloc_gl_buffer<cl_float4>(_particle_count, _vbo_positions);
    _positions_sorted = CLAllocator::alloc_buffer<cl_float4>(_particle_count);
    _positions_predicted = CLAllocator::alloc_buffer<cl_float4>(_particle_count);

    // Upload the particle positions to the device buffer
    CLAllocator::upload_to_gl_buffer(fluid_particles, _positions_unsorted);

    // Initialize buffer for storing the hashes
    _hashes = CLAllocator::alloc_buffer<cl_uint>(_particle_count);

    // Initialize buffer mask
    _mask = CLAllocator::alloc_buffer<cl_int>(_particle_count);

    // Initialize the buffers that hold the velocity of particles
    _velocities_unsorted = CLAllocator::alloc_buffer<cl_float4>(_particle_count, CL_FLOAT4_ZERO);
    _velocities_sorted = CLAllocator::alloc_buffer<cl_float4>(_particle_count);
    _velocities_predicted = CLAllocator::alloc_buffer<cl_float4>(_particle_count);

    _mass_densities = CLAllocator::alloc_buffer<cl_float>(_particle_count);
    _mass_densities_predicted = CLAllocator::alloc_buffer<cl_float>(_particle_count);
    _mass_density_variation = CLAllocator::alloc_buffer<cl_float>(_particle_count);
    _pressures = CLAllocator::alloc_buffer<cl_float>(_particle_count);
    _normals = CLAllocator::alloc_buffer<cl_float4>(_particle_count);

    _particle_force = CLAllocator::alloc_buffer<cl_float4>(_particle_count);
    _pressure_force = CLAllocator::alloc_buffer<cl_float4>(_particle_count);

    // Now initialize the buffers related to holding particles neighbourhood
    _cell_intervals = CLAllocator::alloc_buffer<cl_int2>(_grid->info().cells_count);

    // Initialize the buffer to hold the neighbourhood for each particle
    _neigh_list_length = CLAllocator::alloc_buffer<cl_int>(_particle_count);
    _neighbourhood_list = CLAllocator::alloc_buffer<cl_int>(_particle_count * _max_neigh_list_length);

    _sb_neigh_list_length = CLAllocator::alloc_buffer<cl_int>(_particle_count);
    _sb_neigh_list = CLAllocator::alloc_buffer<cl_int>(_particle_count * _max_neigh_list_length);

    _image_positions = CLAllocator::alloc_1d_image_from_buff(_particle_count, CL_RGBA, _positions_sorted);
    _image_predicted_positions = CLAllocator::alloc_1d_image_from_buff(_particle_count, CL_RGBA, _positions_predicted);
    _image_velocities = CLAllocator::alloc_1d_image_from_buff(_particle_count, CL_RGBA, _velocities_sorted);
    _image_densities = CLAllocator::alloc_1d_image_from_buff(_particle_count, CL_R, _mass_densities);
    _image_normals = CLAllocator::alloc_1d_image_from_buff(_particle_count, CL_RGBA, _normals);
    _image_pressures = CLAllocator::alloc_1d_image_from_buff(_particle_count, CL_R, _pressures);
    
    // Reset and store the references to the shared GL buffers
    _gl_shared_buffers.clear();
    _gl_shared_buffers.push_back(_positions_unsorted);
}

void PCISPHSimulation::_initialize_params(const PhysicsSettings& fluid_settings,
                                          const SimulationSettings& sim_settings) {
    _dt = sim_settings.time_step;
    _max_vel = sim_settings.max_vel;
    _max_iterations = sim_settings.pcisph_max_iterations;
    _density_variation_threshold = sim_settings.pcisph_error_ratio;
    _rest_density = fluid_settings.rest_density;
    _k_viscosity = fluid_settings.k_viscosity;
    _surface_tension = fluid_settings.surface_tension;
    
    _g.s[0] = 0;
    _g.s[1] = -fluid_settings.gravity;
    _g.s[2] = 0;
    _g.s[3] = 0;

    // Estimate the support radius of the kernels
    _particle_radius = sim_settings.fluid_particle_radius;
    _support_radius = sim_settings.fluid_support_radius;
    _sqr_support_radius = _support_radius * _support_radius;

    // Estimate the mass of each particle
    // Yes, each particle has the same, constant mass
    _particle_mass = _rest_density * pow(2.0f * _particle_radius, 3.0f) / 1.15f;
    _st_kernel_main_constant = 32.0f / (M_PI * pow(_support_radius, 9.0f));
    _st_kernel_term_constant = pow(_support_radius, 6.0f) / 64.0f;
}

void PCISPHSimulation::reset(const PhysicsSettings& fluid_settings,
                             const SimulationSettings& sim_settings) {
    // Reset internal params
    _initialize_params(fluid_settings, sim_settings);
    
    _boundary_handler->set_particle_radius(sim_settings.fluid_particle_radius);
    _boundary_handler->set_support_radius(sim_settings.fluid_support_radius);

    _initialize_solver();
}

void PCISPHSimulation::_setup_kernel_params() {   
    if (_particle_count <= 0) {
        cout << "No fluid particles, skiping kernel params setup" << endl;
        return;
    }

    auto grid_info = _grid->info();
    auto default_eval = _smoothing_constants.default_eval(_support_radius);
    auto default_grad = _smoothing_constants.default_grad(_support_radius);
    auto viscosity_lapl = _smoothing_constants.viscosity_lapl(_support_radius);
    auto pressure_grad = _smoothing_constants.pressure_grad(_support_radius);

    _kernel_initial_density->set_arg(0, &_image_positions);
    _kernel_initial_density->set_arg(1, &_mass_densities);
    _kernel_initial_density->set_arg(2, &_neigh_list_length);
    _kernel_initial_density->set_arg(3, &_neighbourhood_list);
    _kernel_initial_density->set_arg(4, &_sb_neigh_list);
    _kernel_initial_density->set_arg(5, &_sb_neigh_list_length);
    if (_boundary_handler->particle_count() > 0) {
        _kernel_initial_density->set_arg(6, &_boundary_handler->_image_positions);
        _kernel_initial_density->set_arg(7, &_boundary_handler->_image_phi);
    }
    else {
        // We bind something so that the kernel does not fail, because 
        // if there are no boundary particles, the image_position and 
        // image_phi are nullptr
        _kernel_initial_density->set_arg(6, &_image_positions);
        _kernel_initial_density->set_arg(7, &_image_positions);
    }
    _kernel_initial_density->set_arg(8, &default_eval);
    _kernel_initial_density->set_local_buffer(9, sizeof(cl_float4));

    _kernel_normals->set_arg(0, &_image_positions);
    _kernel_normals->set_arg(1, &_image_densities);
    _kernel_normals->set_arg(2, &_normals);
    _kernel_normals->set_arg(3, &_neigh_list_length);
    _kernel_normals->set_arg(4, &_neighbourhood_list);
    _kernel_normals->set_arg(5, &default_grad);
    _kernel_normals->set_local_buffer(6, sizeof(cl_float4));
    _kernel_normals->set_local_buffer(7, sizeof(cl_float));

    _kernel_initial_forces->set_arg(0, &_image_positions);
    _kernel_initial_forces->set_arg(1, &_image_velocities);
    _kernel_initial_forces->set_arg(2, &_image_densities);
    _kernel_initial_forces->set_arg(3, &_image_normals);
    _kernel_initial_forces->set_arg(4, &_particle_force);   
    _kernel_initial_forces->set_arg(5, &_k_viscosity);
    _kernel_initial_forces->set_arg(6, &_surface_tension);
    _kernel_initial_forces->set_arg(7, &_neigh_list_length);
    _kernel_initial_forces->set_arg(8, &_neighbourhood_list);
    _kernel_initial_forces->set_arg(9, &viscosity_lapl);
    _kernel_initial_forces->set_arg(10, &_st_kernel_main_constant);
    _kernel_initial_forces->set_arg(11, &_st_kernel_term_constant);
    _kernel_initial_forces->set_local_buffer(12, sizeof(cl_float4));
    _kernel_initial_forces->set_local_buffer(13, sizeof(cl_float4));
    _kernel_initial_forces->set_local_buffer(14, sizeof(cl_float));
    _kernel_initial_forces->set_local_buffer(15, sizeof(cl_float4));

    _kernel_predict_vel_n_pos->set_arg(0, &_positions_sorted);
    _kernel_predict_vel_n_pos->set_arg(1, &_velocities_sorted);
    _kernel_predict_vel_n_pos->set_arg(2, &_particle_force);
    _kernel_predict_vel_n_pos->set_arg(3, &_pressure_force);
    _kernel_predict_vel_n_pos->set_arg(4, &_positions_predicted);
    _kernel_predict_vel_n_pos->set_arg(5, &_velocities_predicted);
    _kernel_predict_vel_n_pos->set_arg(6, &_dt);
    _kernel_predict_vel_n_pos->set_arg(7, &_g);
    _kernel_predict_vel_n_pos->set_arg(8, &_max_vel);
    _kernel_predict_vel_n_pos->set_arg(9, &_container_size);

    _kernel_predict_pos->set_arg(0, &_positions_sorted);
    _kernel_predict_pos->set_arg(1, &_velocities_sorted);
    _kernel_predict_pos->set_arg(2, &_particle_force);
    _kernel_predict_pos->set_arg(3, &_pressure_force);
    _kernel_predict_pos->set_arg(4, &_positions_predicted);
    _kernel_predict_pos->set_arg(5, &_dt);
    _kernel_predict_pos->set_arg(6, &_g);
    _kernel_predict_pos->set_arg(7, &_max_vel);
    _kernel_predict_pos->set_arg(8, &_container_size);

    _kernel_update_pressure->set_arg(0, &_image_predicted_positions);
    _kernel_update_pressure->set_arg(1, &_mass_density_variation);
    _kernel_update_pressure->set_arg(2, &_pressures);
    _kernel_update_pressure->set_arg(3, &_density_scale_factor);
    _kernel_update_pressure->set_arg(4, &_neigh_list_length);
    _kernel_update_pressure->set_arg(5, &_neighbourhood_list);
    _kernel_update_pressure->set_arg(6, &_sb_neigh_list);
    _kernel_update_pressure->set_arg(7, &_sb_neigh_list_length);
    if (_boundary_handler->particle_count() > 0) {
        _kernel_update_pressure->set_arg(8, &_boundary_handler->_image_positions);
        _kernel_update_pressure->set_arg(9, &_boundary_handler->_image_phi);
    }
    else {
        // We bind something so that the kernel does not fail, because 
        // if there are no boundary particles, the image_position and 
        // image_phi are nullptr
        _kernel_update_pressure->set_arg(8, &_image_positions);
        _kernel_update_pressure->set_arg(9, &_image_positions);
    }
    _kernel_update_pressure->set_arg(10, &default_eval);
    _kernel_update_pressure->set_local_buffer(11, sizeof(cl_float4));
    
    _kernel_compute_pressure_force->set_arg(0, &_image_positions);
    _kernel_compute_pressure_force->set_arg(1, &_image_densities);
    _kernel_compute_pressure_force->set_arg(2, &_image_pressures);
    _kernel_compute_pressure_force->set_arg(3, &_pressure_force);  
    _kernel_compute_pressure_force->set_arg(4, &grid_info);
    _kernel_compute_pressure_force->set_arg(5, &_neigh_list_length);
    _kernel_compute_pressure_force->set_arg(6, &_neighbourhood_list);
    _kernel_compute_pressure_force->set_arg(7, &_sb_neigh_list);
    _kernel_compute_pressure_force->set_arg(8, &_sb_neigh_list_length);
    if (_boundary_handler->particle_count() > 0) {
        _kernel_compute_pressure_force->set_arg(9, &_boundary_handler->_image_positions);
        _kernel_compute_pressure_force->set_arg(10, &_boundary_handler->_image_phi);
    }
    else {
        // We bind something so that the kernel does not fail, because 
        // if there are no boundary particles, the image_position and 
        // image_phi are nullptr
        _kernel_compute_pressure_force->set_arg(9, &_image_positions);
        _kernel_compute_pressure_force->set_arg(10, &_image_positions);
    }
    _kernel_compute_pressure_force->set_arg(11, &pressure_grad);
    _kernel_compute_pressure_force->set_local_buffer(12, sizeof(cl_float4));
    _kernel_compute_pressure_force->set_local_buffer(13, sizeof(cl_float));
    _kernel_compute_pressure_force->set_local_buffer(14, sizeof(cl_float));
}

void PCISPHSimulation::_release_buffers() {
    CLAllocator::release_buffer(_image_positions);
    CLAllocator::release_buffer(_image_predicted_positions);
    CLAllocator::release_buffer(_image_velocities);
    CLAllocator::release_buffer(_image_densities);
    CLAllocator::release_buffer(_image_normals);
    CLAllocator::release_buffer(_image_pressures);

    CLAllocator::release_buffer(_positions_unsorted);
    CLAllocator::release_buffer(_positions_sorted);
    CLAllocator::release_buffer(_positions_predicted);
    CLAllocator::release_buffer(_hashes);
    CLAllocator::release_buffer(_mask);
    CLAllocator::release_buffer(_velocities_unsorted);
    CLAllocator::release_buffer(_velocities_sorted);
    CLAllocator::release_buffer(_velocities_predicted);
    CLAllocator::release_buffer(_mass_densities);
    CLAllocator::release_buffer(_mass_densities_predicted);
    CLAllocator::release_buffer(_mass_density_variation);
    CLAllocator::release_buffer(_pressures);
    CLAllocator::release_buffer(_particle_force);
    CLAllocator::release_buffer(_pressure_force);
    CLAllocator::release_buffer(_cell_intervals);
    CLAllocator::release_buffer(_neigh_list_length);
    CLAllocator::release_buffer(_neighbourhood_list);
    CLAllocator::release_buffer(_normals);
    CLAllocator::release_buffer(_sb_neigh_list);
    CLAllocator::release_buffer(_sb_neigh_list_length);
}

PCISPHSimulation::~PCISPHSimulation() {
    _release_buffers();
}

void PCISPHSimulation::_deduce_density_scale_factor() {
    float spiky_grad_constant = _smoothing_constants.pressure_grad(_support_radius);
    float poly6_grad_constant = _smoothing_constants.default_grad(_support_radius);
    float beta = 2.0f * pow((_dt * _particle_mass) / _rest_density, 2.0f);
    float value_sum[] = {0.f, 0.f, 0.f};
    float value_spiky_sum[] = {0.f, 0.f, 0.f};
    float value_dot_value_sum = 0.f;

    for(float x = -_support_radius - _particle_radius; x <= _support_radius + _particle_radius; x += 2.0f * _particle_radius) {
        for(float y = -_support_radius - _particle_radius; y <= _support_radius + _particle_radius; y += 2.0f * _particle_radius) {
            for(float z = -_support_radius - _particle_radius; z <= _support_radius + _particle_radius; z += 2.0f * _particle_radius) {
                auto r2 = (x * x + y * y + z * z);
                auto r_norm = sqrt(r2);

                if(r2 < _sqr_support_radius) {
                    float spiky_factor = spiky_grad_constant * (_support_radius - r_norm)*(_support_radius - r_norm) * (1.0/r_norm);
                    float factor = poly6_grad_constant * (_sqr_support_radius - r2)*(_sqr_support_radius - r2);
                    
                    float spiky_value[] = {spiky_factor*x, spiky_factor*y ,spiky_factor*z};
                    float value[] = {factor*x, factor*y, factor*z};

                    for(int i = 0; i < 3; i++) {
                        // add value
                        value_sum[i] += value[i];
                        value_spiky_sum[i] += spiky_value[i];

                        // dot product of value
                        value_dot_value_sum += value[i] * spiky_value[i];
                    }
                }
            }
        }
    }

    // dot product of value sum
    float value_sum_dot_value_sum = 0.f;
    for(int i = 0; i < 3; i++) {
        value_sum_dot_value_sum += value_spiky_sum[i] * value_sum[i];
    }
    _density_scale_factor = -1.0f / (beta * (-value_sum_dot_value_sum - value_dot_value_sum));
}

float PCISPHSimulation::_get_max_density_variation() {
    cl_float max_density;
    clmax<cl_float>(_mass_density_variation, _particle_count, max_density);

    return max_density;
}

void PCISPHSimulation::add_boundary(const shared_ptr<RigidBody> boundary, 
                                    const string& boundary_id,
                                    bool can_move) {
    _boundary_handler->add_boundary(boundary, 
                                    boundary_id, 
                                    can_move);

    // We reset all kernel params so that the static boundary buffers are
    // updated
    _build_kernels();
}

void PCISPHSimulation::_build_kernels() {
    // First, the program must be compiled
    CLCompiler compiler;
    compiler.add_file_source("kernels/pcisph.cl");
    compiler.add_file_source("kernels/normals.cl");
    compiler.add_build_option("-cl-std=CL1.2");
    compiler.add_build_option("-cl-fast-relaxed-math");
    compiler.add_build_option("-cl-mad-enable");
    compiler.add_include_path("kernels");
    
    // Define the constants for the program
    compiler.define_constant("PARTICLE_COUNT", _particle_count);
    compiler.define_constant("SUPPORT_RADIUS", _support_radius);
    compiler.define_constant("PARTICLE_MASS", _particle_mass);
    compiler.define_constant("REST_DENSITY", _rest_density);
    compiler.define_constant("USE_MULLER_KERNELS");
    if (_boundary_handler->particle_count()) {
        compiler.define_constant("COMPUTE_BOUNDARY", 1);
    }

    // Compile!
    _program = compiler.build();

    // Now reinitialize all kernel instances
    _kernel_initial_density = _program->get_kernel("compute_initial_density");
    _kernel_initial_forces = _program->get_kernel("compute_initial_forces");
    _kernel_predict_vel_n_pos = _program->get_kernel("predict_vel_n_pos");
    _kernel_predict_pos = _program->get_kernel("predict_pos");
    _kernel_update_pressure = _program->get_kernel("update_pressure");
    _kernel_compute_pressure_force = _program->get_kernel("compute_pressure_force");
    _kernel_normals = _program->get_kernel("compute_normals");

    _setup_kernel_params();
}

void PCISPHSimulation::add_volume(const std::shared_ptr<FluidVolume> volume) {
    _volumes.push_back(volume);

    // Reinitialize the whole solver to hold the new particles
    _initialize_solver();
}

void PCISPHSimulation::_initialize_solver() {
    // Reset grid cell size with the new support radius
    _grid->set_cell_size(_support_radius);

    // Reinitialize all buffers
    _initialize_buffers();

    // Rebuild all kernels
    _build_kernels();

    // Deduce the density variation scaling factor of PCISPH algorithm
    _deduce_density_scale_factor();

    // Configure kernel parameters once, call them later many times
    _setup_kernel_params();

    // Relax particle position, and reset velocities
    _simulate_pcisph_step(_min_iterations, 10000);
    CLAllocator::fill_buffer(_velocities_unsorted, CL_FLOAT4_ZERO, _particle_count);
}

int PCISPHSimulation::boundary_particle_count() const {
    return _boundary_handler->particle_count();
}

void PCISPHSimulation::set_rect_limits(float width, float height, float depth) {
    _container_size.s[0] = width;
    _container_size.s[1] = height;
    _container_size.s[2] = depth;
    _container_size.s[3] = 0.0f;

    _setup_kernel_params();
}