#include "settings.h"
#include "fileparser.h"
#include "runtimeexception.h"
#include <fstream>
#include <cstdlib>

using namespace std;

unique_ptr<GraphicsSettings> Settings::_graphics = nullptr;
unique_ptr<SimulationSettings> Settings::_simulation = nullptr;
unique_ptr<PhysicsSettings> Settings::_physics = nullptr;
string Settings::_scene_file = "";

void Settings::load(const string& config_file, const string& scene_file) {
    _graphics   = unique_ptr<GraphicsSettings>(new GraphicsSettings());
    _simulation = unique_ptr<SimulationSettings>(new SimulationSettings());
    _physics    = unique_ptr<PhysicsSettings>(new PhysicsSettings());
    _scene_file = scene_file;

    FileParser parser(config_file);

    // Simulation settings
    _simulation->time_step              = atof(parser.option("time_step").c_str());
    _simulation->max_vel                = atof(parser.option("max_vel").c_str());
    _simulation->fluid_particle_radius  = atof(parser.option("fluid_particle_radius").c_str());
    _simulation->fluid_support_radius   = atof(parser.option("fluid_support_radius").c_str());
    _simulation->pcisph_max_iterations  = atoi(parser.option("pcisph_max_iterations").c_str());
    _simulation->pcisph_error_ratio     = atof(parser.option("pcisph_error_ratio").c_str());
    
    auto method = parser.option("method");
    if (method == "wcsph") {
        _simulation->sim_method = SimulationSettings::Method::WCSPH;
    }
    else if (method == "pcisph") {
        _simulation->sim_method = SimulationSettings::Method::PCISPH;
    }
    else {
        throw RunTimeException("Unknown simulation method '" + method + "'!");
    }

    // Physics settings
    _physics->rest_density          = atof(parser.option("rest_density").c_str());
    _physics->k_viscosity           = atof(parser.option("k_viscosity").c_str());
    _physics->gravity               = atof(parser.option("gravity").c_str());
    _physics->gas_stiffness         = atof(parser.option("gas_stiffness").c_str());
    _physics->surface_tension       = atof(parser.option("surface_tension").c_str());

    // Graphics settings
    auto render_method = parser.option("render_method");
    if (render_method == "particles") {
        _graphics->render_method = RenderMethod::PARTICLES;
    }
    else if (render_method == "screenspace") {
        _graphics->render_method = RenderMethod::SCREEN_SPACE;
    }
    else {
        throw RunTimeException("Unknown rendering method '" + render_method + "'!");
    }
}

GraphicsSettings& Settings::graphics() {
    if (_graphics != nullptr) {
        return *_graphics;
    }
    else {
        throw RunTimeException("Settings not initialized!");
    }
}

SimulationSettings& Settings::simulation() {
    if (_simulation != nullptr) {
        return *_simulation;
    }
    else {
        throw RunTimeException("Settings not initialized!");
    }
}

PhysicsSettings& Settings::physics() {
    if (_physics != nullptr) {
        return *_physics;
    }
    else {
        throw RunTimeException("Settings not initialized!");
    }
}

string Settings::scene_file() {
    return _scene_file;
}

void Settings::update_graphics(const GraphicsSettings& g_settings) {
    *_graphics = g_settings;
}

void Settings::update_simulation(const SimulationSettings& s_settings) {
    *_simulation = s_settings;
}

void Settings::update_physics(const PhysicsSettings& p_settings) {
    *_physics = p_settings;
}

void Settings::update(const GraphicsSettings& g_settings,
                      const SimulationSettings& s_settings,
                      const PhysicsSettings& p_settings) {
    *_graphics = g_settings;
    *_simulation = s_settings;
    *_physics = p_settings; 
}