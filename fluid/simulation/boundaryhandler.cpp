#define CL_USE_DEPRECATED_OPENCL_1_1_APIS   1 // Needed for clogs to compile

#include "boundaryhandler.h"
#include "opencl/clallocator.h"
#include "opencl/clenvironment.h"
#include "opencl/algorithms/clsort.h"
#include "opencl/algorithms/clshuffle.h"
#include "opencl/algorithms/clreduce.h"
#include "mullerconstants.h"

using namespace std;

BoundaryHandler::BoundaryHandler(const Grid& grid,
                                 float particle_radius,
                                 float support_radius,
                                 float rest_density,
                                 float phi_coefficient) :
_count(0),
_particle_spacing(2 * particle_radius),
_support_radius(support_radius),
_rest_density(rest_density),
_phi_coefficient(phi_coefficient),
_grid(grid),
_raw_positions(nullptr),
_unsorted_positions(nullptr),
_sorted_positions(nullptr),
_velocities(nullptr),
_sorted_velocities(nullptr),
_cell_intervals(nullptr),
_phi(nullptr),
_sorted_phi(nullptr),
_fluid_force(nullptr),
_hashes(nullptr),
_mask(nullptr),
_image_positions(nullptr),
_image_phi(nullptr) {

}

BoundaryHandler::~BoundaryHandler() {
    _release();
}

void BoundaryHandler::_release() {
    CLAllocator::release_buffer(_image_phi);
    CLAllocator::release_buffer(_image_positions);
    
    CLAllocator::release_buffer(_raw_positions);
    CLAllocator::release_buffer(_unsorted_positions);
    CLAllocator::release_buffer(_sorted_positions);
    CLAllocator::release_buffer(_velocities);
    CLAllocator::release_buffer(_sorted_velocities);
    CLAllocator::release_buffer(_cell_intervals);
    CLAllocator::release_buffer(_phi);
    CLAllocator::release_buffer(_sorted_phi);
    CLAllocator::release_buffer(_fluid_force);
    CLAllocator::release_buffer(_hashes);
    CLAllocator::release_buffer(_mask);

    for (auto& rb_info : _bodies) {
        CLAllocator::release_buffer(rb_info.torque);
        CLAllocator::release_buffer(rb_info.forces);
    }
}

void BoundaryHandler::set_particle_radius(float particle_radius) {
    if (_count > 0) {
        float new_particle_spacing = 2 * particle_radius;
        if (_particle_spacing != new_particle_spacing) {
            _particle_spacing = new_particle_spacing;
            // Must rebuild all surfaces
            _sample_surfaces();
        }
    }
}

float BoundaryHandler::particle_radius() const {
    return _particle_spacing / 2.0f;
}

void BoundaryHandler::set_support_radius(float support_radius) {
    if (_count > 0) {
        if (_support_radius != support_radius) {
            _support_radius = support_radius;
            // Must rebuild all surfaces
            _sample_surfaces();
        }
    }
}

float BoundaryHandler::support_radius() const {
    return _support_radius;
}

void BoundaryHandler::add_boundary(shared_ptr<RigidBody> boundary,
                                   const string& boundary_id,
                                   bool can_move) {
    // Add the new body information to the internal collection
    _SurfaceInfo info;
    info.raw_sub_buffer = nullptr;
    info.transformed_sub_buffer = nullptr;
    info.vel_sub_buffer = nullptr;
    info.forces = nullptr;
    info.torque = nullptr;
    info.body = boundary;
    info.can_move = can_move;
    _bodies.push_back(info);

    // Now rebuild all internal buffers
    _sample_surfaces();
}

void BoundaryHandler::_clear() {
    _release();
    _particles.clear();
    _count = 0;
}

void BoundaryHandler::_sample_surfaces() {
    _clear();

    // Now resample all bodies
    for (auto& s_info : _bodies) {
        auto sampling = s_info.body->surface_sampling(_particle_spacing);
        
        // Copy the new particles
        for (auto& pos : sampling) {
            cl_float4 cl_pos;
            cl_pos.s[0] = pos.x();
            cl_pos.s[1] = pos.y();
            cl_pos.s[2] = pos.z();
            cl_pos.s[3] = 1;
            _particles.push_back(cl_pos);
        }
        
        s_info.particle_count = sampling.size();
        s_info.buff_origin = _count;
        s_info.buff_size = sampling.size();

        // We update the total number of particles of this boundary handler
        _count = _particles.size();
    }  
    
    _poly6_eval = MullerConstants::default_eval(_support_radius);

    _alloc_buffers();

    _build_kernels();

    _compute_boundary_phi();

    sync(true);
}

void BoundaryHandler::_alloc_buffers() {
    cl_int err;
    _raw_positions = CLAllocator::alloc_buffer<cl_float4>(_count, _particles);
    _unsorted_positions = CLAllocator::alloc_buffer<cl_float4>(_count, _particles);
    _sorted_positions = CLAllocator::alloc_buffer<cl_float4>(_count);
    _velocities = CLAllocator::alloc_buffer<cl_float4>(_count);
    _sorted_velocities = CLAllocator::alloc_buffer<cl_float4>(_count);
    _cell_intervals = CLAllocator::alloc_buffer<cl_int2>(_grid.cell_count());
    _phi = CLAllocator::alloc_buffer<cl_float>(_count);
    _sorted_phi = CLAllocator::alloc_buffer<cl_float>(_count);
    _fluid_force = CLAllocator::alloc_buffer<cl_float4>(_count);
    _hashes = CLAllocator::alloc_buffer<cl_uint>(_count);
    _mask = CLAllocator::alloc_buffer<cl_int>(_count);

    
    //_image_velocities = CLAllocator::alloc_1d_image_from_buff(_count, CL_RGBA, _sorted_velocities);
    _image_phi = CLAllocator::alloc_1d_image_from_buff(_count, CL_R, _sorted_phi);
    _image_positions = CLAllocator::alloc_1d_image_from_buff(_count, CL_RGBA, _sorted_positions);

    // Also, reallocate all the sub-buffers of each body
    for (auto& rb_info : _bodies) {
        cl_buffer_region r;
        r.origin = rb_info.buff_origin * sizeof(cl_float4);
        r.size = rb_info.buff_size * sizeof(cl_float4);

        // Create subbuffers
        rb_info.raw_sub_buffer = clCreateSubBuffer(_raw_positions,
                                                   CL_MEM_READ_WRITE,
                                                   CL_BUFFER_CREATE_TYPE_REGION,
                                                   &r,
                                                   &err);
        CLError::check(err);

        rb_info.transformed_sub_buffer = clCreateSubBuffer(_unsorted_positions,
                                                           CL_MEM_READ_WRITE,
                                                           CL_BUFFER_CREATE_TYPE_REGION,
                                                           &r,
                                                           &err);
        CLError::check(err);

        rb_info.vel_sub_buffer = clCreateSubBuffer(_velocities,
                                                   CL_MEM_READ_WRITE,
                                                   CL_BUFFER_CREATE_TYPE_REGION,
                                                   &r,
                                                   &err);
        CLError::check(err);

        r.origin = rb_info.buff_origin * sizeof(cl_float);
        r.size = rb_info.buff_size * sizeof(cl_float);
        
        rb_info.phi_sub_buffer = clCreateSubBuffer(_phi,
                                                   CL_MEM_READ_WRITE,
                                                   CL_BUFFER_CREATE_TYPE_REGION,
                                                   &r,
                                                   NULL);

        rb_info.forces = CLAllocator::alloc_buffer<cl_float4>(rb_info.particle_count);
        rb_info.torque = CLAllocator::alloc_buffer<cl_float4>(rb_info.particle_count);
    }
}

void BoundaryHandler::_sort_positions() {
    // Compute the hashes for all boundary particles
    _grid.compute_hashes(_unsorted_positions,
                         _hashes,
                         _mask,
                         _count);

    // Now sort the hashes buffer
    clsort<cl_uint, cl_int>(_hashes, _mask, _count);

    // Now sort the particles with the mask
    clshuffle<cl_float4>(_unsorted_positions, _mask, _sorted_positions, _count);
    //clshuffle<cl_float4>(_velocities, _mask, _sorted_velocities, _count);
    clshuffle<cl_float>(_phi, _mask, _sorted_phi, _count);

    // Last step is to compute the intervals for the boundary particles
    _grid.compute_cell_intervals(_sorted_positions,
                                 _hashes,
                                 _cell_intervals,
                                 _count);
}

void BoundaryHandler::_compute_boundary_phi() {
    for (auto& rb_info : _bodies) {
        _kernel_boundary_phi->set_arg(0, &rb_info.transformed_sub_buffer);
        _kernel_boundary_phi->set_arg(1, &rb_info.phi_sub_buffer);
        _kernel_boundary_phi->set_arg(2, &_rest_density);
        _kernel_boundary_phi->set_arg(3, &_poly6_eval);
        _kernel_boundary_phi->set_arg(4, &_phi_coefficient);
        _kernel_boundary_phi->set_arg(5, &rb_info.particle_count);

        CLError::check(_kernel_boundary_phi->run(rb_info.particle_count));
    }
}

void BoundaryHandler::sync(bool sync_all) {
    if (_count == 0) {
        // Nothing to sync
        return;
    }

    for (auto& rb_info : _bodies) {
        if (rb_info.can_move || sync_all) {
            auto rot_matrix = rb_info.body->transform().getBasis();
            auto row0 = rot_matrix[0];
            auto row1 = rot_matrix[1];
            auto row2 = rot_matrix[2];

            cl_float4 cl_row0 = {row0.x(), row0.y(), row0.z()};
            cl_float4 cl_row1 = {row1.x(), row1.y(), row1.z()};
            cl_float4 cl_row2 = {row2.x(), row2.y(), row2.z()};

            auto angular_vel = rb_info.body->angular_vel();
            auto linear_vel = rb_info.body->linear_vel();
            auto cm_pos = rb_info.body->position();

            cl_float4 cl_angular_vel = {angular_vel.x(), angular_vel.y(), angular_vel.z(), 0.0f};
            cl_float4 cl_linear_vel = {linear_vel.x(), linear_vel.y(), linear_vel.z(), 0.0f};
            cl_float4 cl_cm_position = {cm_pos.x(), cm_pos.y(), cm_pos.z(), 1.0f};

            _kernel_transform_particles->set_arg(0, &rb_info.raw_sub_buffer);
            _kernel_transform_particles->set_arg(1, &rb_info.transformed_sub_buffer);
            _kernel_transform_particles->set_arg(2, &rb_info.vel_sub_buffer);
            _kernel_transform_particles->set_arg(3, &cl_row0);
            _kernel_transform_particles->set_arg(4, &cl_row1);
            _kernel_transform_particles->set_arg(5, &cl_row2);
            _kernel_transform_particles->set_arg(6, &cl_linear_vel);
            _kernel_transform_particles->set_arg(7, &cl_angular_vel);
            _kernel_transform_particles->set_arg(8, &cl_cm_position);
            _kernel_transform_particles->set_arg(9, &rb_info.particle_count);
        
            auto err = _kernel_transform_particles->run(rb_info.particle_count);

            CLError::check(err);
        }
    }

    _sort_positions();
}

void BoundaryHandler::apply_fluid_forces(cl_mem fluid_particles,
                                         cl_mem fluid_velocities,
                                         cl_mem fluid_densities,
                                         cl_mem fluid_pressures,
                                         cl_mem fluid_cell_intervals,
                                         float fluid_particle_mass) {
    if (_count == 0) {
        return;
    }

    GridInfo grid_info = _grid.info();
    float support_radius = grid_info.cell_size;
    float spiky_grad = -45.0f / (M_PI * pow(_support_radius, 6));
    float visc_lapl = 45.0f / (M_PI * pow(_support_radius, 6));
    _kernel_fluid_force->set_arg(2, &fluid_particles);
    _kernel_fluid_force->set_arg(3, &fluid_densities);
    _kernel_fluid_force->set_arg(4, &fluid_pressures);
    _kernel_fluid_force->set_arg(5, &fluid_cell_intervals);
    _kernel_fluid_force->set_arg(6, &fluid_particle_mass);

    _kernel_fluid_force->set_arg(7, &spiky_grad);
    _kernel_fluid_force->set_arg(8,&grid_info);
    _kernel_fluid_force->set_arg(13, &fluid_velocities);
    _kernel_fluid_force->set_arg(15, &visc_lapl);
   
    for (auto& rb_info : _bodies) {
        if (rb_info.body->mass() > 0.0) {
            auto cm_pos = rb_info.body->position();

            cl_float4 cm = {cm_pos.x(), cm_pos.y(), cm_pos.z(), 1};

            _kernel_fluid_force->set_arg(0, &rb_info.transformed_sub_buffer);
            _kernel_fluid_force->set_arg(1, &rb_info.phi_sub_buffer);
            _kernel_fluid_force->set_arg(9, &rb_info.particle_count);
            _kernel_fluid_force->set_arg(10, &rb_info.forces);
            _kernel_fluid_force->set_arg(11, &rb_info.torque);
            _kernel_fluid_force->set_arg(12, &cm);
            _kernel_fluid_force->set_arg(14, &rb_info.vel_sub_buffer);

            auto err = _kernel_fluid_force->run(rb_info.particle_count);
            CLError::check(err);

            cl_float4 f = {{0, 0, 0, 0}};
            cl_float4 t = {{0, 0, 0, 0}};
            err = clsum(rb_info.forces, rb_info.particle_count, f);
            CLError::check(err);
            err = clsum(rb_info.torque, rb_info.particle_count, t);
            CLError::check(err);

            rb_info.body->apply_force_n_torque(btVector3(f.s[0], f.s[1], f.s[2]),
                                               btVector3(t.s[0], t.s[1], t.s[2]));
        }
    }
}

void BoundaryHandler::build_neighbourhood(cl_mem ref_positions,
                                          cl_mem neigh_list,
                                          cl_mem neigh_list_length,
                                          int max_list_length,
                                          int particle_count) const {
    if (_count > 0) {
        // Only perform the build if there are boundary particles available
        // Otherwise this will fail because _cell_intervals buffer will not 
        // be initialized
        _grid.compute_neigh_list(ref_positions,
                                 _image_positions,
                                 _cell_intervals,
                                 neigh_list,
                                 neigh_list_length,
                                 max_list_length,
                                 particle_count);
    }
    else {
        CLAllocator::fill_buffer(neigh_list_length, 0, particle_count);
    }
}

cl_mem* BoundaryHandler::positions_buffer() {
    return &_sorted_positions;
}

cl_mem* BoundaryHandler::velocities_buffer() {
    return &_sorted_velocities;
}

cl_mem* BoundaryHandler::phi_buffer() {
    return &_sorted_phi;
}

cl_mem* BoundaryHandler::positions_image() {
    return &_image_positions;
}

cl_mem* BoundaryHandler::phi_image() {
    return &_image_phi;
}

int BoundaryHandler::particle_count() const {
    return _count;
}

void BoundaryHandler::set_phi_coefficient(float phi_coeff) {
    _phi_coefficient = phi_coeff;

    // Now rebuild all internal buffers
    _sample_surfaces();
}

void BoundaryHandler::_build_kernels() {
    CLCompiler compiler;
    compiler.add_file_source("kernels/boundary.cl");
    compiler.add_build_option("-cl-std=CL1.2");
    compiler.add_build_option("-cl-fast-relaxed-math");
    compiler.add_build_option("-cl-mad-enable");
    compiler.add_include_path("kernels");
    compiler.define_constant("SUPPORT_RADIUS", _support_radius);
    compiler.define_constant("BOUNDARY_PARTICLE_COUNT", _count);
    compiler.define_constant("USE_MULLER_KERNELS");
    _program = compiler.build();

    _kernel_boundary_phi = _program->get_kernel("compute_boundary_phi");
    _kernel_fluid_force = _program->get_kernel("compute_fluid_force");
    _kernel_transform_particles = _program->get_kernel("transform_particles");
}
