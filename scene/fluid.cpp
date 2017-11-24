#include "fluid.h"
#include "fluid/simulation/fluidsimulation.h"
#include "fluid/render/sspacefluidrenderer.h"
#include "fluid/render/particlesrenderer.h"
#include "opengl/glutils.h"

#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

Fluid::Fluid(int viewport_width,
             int viewport_height,
             const PhysicsSettings& fluid_settings,
             const SimulationSettings& sim_settings,
             const GraphicsSettings& g_settings) :
_viewport_w(viewport_width),
_viewport_h(viewport_height) {
    auto& gl = OpenGLFunctions::getFunctions();

    //Default color
    set_color(9/256.0, 71/256.0, 128/256.0);

    // Before any other initialization, create the most important VBO to hold
    // particles locations
    gl.glGenBuffers(1, &_vbo_fluid_particles);
    // This one is not being used right now
    gl.glGenBuffers(1, &_vbo_boundary_particles); 

    // Now initialize the simulation
    _simulation = FluidSimulationFactory::build_simulation(fluid_settings, 
                                                           sim_settings, 
                                                           _vbo_fluid_particles);

    _init_renderer(fluid_settings, sim_settings, g_settings);
}

Fluid::~Fluid() {

}

void Fluid::simulate() {
    // Simulate fluid state and then render it
    _simulation->simulate();
}

void Fluid::render(const Camera& camera,
                   const vector<DirectionalLight>& dir_lights,
                   GLuint dest_fbo,
                   GLuint background_texture,
                   GLuint bkg_depth_texture,
                   GLuint cube_map_texture) {
    // Load the projection and modelview matrixs
    _qt_transformation.setToIdentity();
    //_qt_transformation.rotate(_rotation);
    _qt_transformation.translate(_transform.getOrigin().x(),
                                 _transform.getOrigin().y(),
                                 _transform.getOrigin().z());
    QMatrix4x4 mv_matrix = camera.transformation() * _qt_transformation;

    _renderer->render(camera,
                      dir_lights,
                      mv_matrix,
                      dest_fbo,
                      background_texture,
                      bkg_depth_texture,
                      cube_map_texture);
}

FluidSimulation& Fluid::simulation() {
    return *_simulation;
}

void Fluid::set_color(float r, float g, float b, float alpha) {
    _color.setRedF(r);
    _color.setGreenF(g);
    _color.setBlueF(b);
    _color.setAlphaF(alpha);
}

void Fluid::set_color(const QColor &c) {
    _color = c;
}

QColor Fluid::color() const {
    return _color;
}

void Fluid::reset(const SimulationSettings& s_settings,
                  const PhysicsSettings& p_settings,
                  const GraphicsSettings& g_settings) {
    
    _simulation->reset(p_settings, s_settings);
    _init_renderer(p_settings, s_settings, g_settings);
}

void Fluid::_init_renderer(const PhysicsSettings& p_settings,
                           const SimulationSettings& s_settings,
                           const GraphicsSettings& g_settings) {
    if (g_settings.render_method == SCREEN_SPACE) {
        _renderer = make_unique<SSpaceFluidRenderer>(_viewport_w,
                                                     _viewport_h,
                                                     g_settings.sspace_filter_iterations,
                                                     _simulation->particle_count(),
                                                     _vbo_fluid_particles);
    }
    else if (g_settings.render_method == PARTICLES) {
        _renderer = make_unique<ParticlesRenderer>(_viewport_w,
                                                   _viewport_h,
                                                   _simulation->particle_count(),
                                                   _vbo_fluid_particles);
    }
}

void Fluid::add_boundary(const std::shared_ptr<RigidBody> boundary, 
                         const string& id,
                         bool can_move) {
    _simulation->add_boundary(boundary, id, can_move);
    _renderer->reset(_simulation->particle_count());
}

int Fluid::particle_count() const {
    return _simulation->particle_count();
}

int Fluid::boundary_particle_count() const {
    return _simulation->boundary_particle_count();
}

void Fluid::add_volume(std::shared_ptr<FluidVolume> volume) {
    _simulation->add_volume(volume);
    _renderer->reset(_simulation->particle_count());
}

void Fluid::set_rect_limits(float width, float height, float depth) {
    _simulation->set_rect_limits(width, height, depth);
}