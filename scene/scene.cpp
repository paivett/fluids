#include "scene.h"
#include "opengl/openglfunctions.h"
#include "opengl/glutils.h"
#include "runtimeexception.h"
#include "external/json/json11.hpp"
#include "scene/fluid.h"
#include "scene/fishtank.h"
#include "scene/cube.h"
#include "scene/sphere.h"
#include "scene/wall.h"
#include "scene/model.h"
#include "fluid/simulation/boxvolume.h"

#include <QImage>
#include <QGLWidget>
#include <fstream>
#include <iostream>
#include <cmath>

// Bullets physics
#include <LinearMath/btVector3.h>

using namespace std;

Scene::Scene(int viewport_width, 
             int viewport_height): 
_viewport_w(viewport_width), 
_viewport_h(viewport_height) {
    //_initialize_scene_fbo(viewport_width, viewport_height);
    _initialize_fbo(viewport_width, 
                    viewport_height,
                    _bkg_fbo,
                    _bkg_color_tex,
                    _bkg_depth_tex);
    
    _intialize_skybox();
    _intialize_bullets();
}

Scene::~Scene() {
    delete _bt_world;
    delete _bt_solver;
    delete _bt_dispatcher;
    delete _bt_collision_configuration;
    delete _bt_broadphase;
}

Camera& Scene::camera() {
    return _camera;
}

void Scene::add_object(const string& oid, shared_ptr<SceneObject> o, bool is_translucent) {
    if (is_translucent) {
        _translucent_objects.push_back(o);
    }
    else {
        _solid_objects.push_back(o);
    }

    _scene_objects[oid] = o;
    _initial_positions[oid] = o->position();
    _initial_rotations[oid] = o->rotation();
}

void Scene::add_rigid_body(const string& oid, shared_ptr<RigidBody> o, bool is_translucent) {
    add_object(oid,
               dynamic_pointer_cast<SceneObject>(o),
               is_translucent);
    auto rg = o->_bt_rigid_body;
    _bt_world->addRigidBody(rg);
}

shared_ptr<SceneObject> Scene::get_object(const string& oid) {
    return _scene_objects[oid];
}

void Scene::render(float dt, GLuint dest_fbo) {
    auto& gl = OpenGLFunctions::getFunctions();

    // Bind background framebuffer and clear it
    gl.glBindFramebuffer(GL_FRAMEBUFFER, _bkg_fbo);
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _render_skybox();

    // Scene graph is implemented as a map.
    // In the future, it may be a more complex structure as a BSP
    for (auto it = _solid_objects.begin(); it != _solid_objects.end(); ++it) {
        (*it)->render(_camera, _dir_lights, _bkg_fbo, _bkg_color_tex, _bkg_depth_tex, _skybox_texture);
    }

    _copy_fbo(_bkg_fbo, dest_fbo);

    for (auto it = _translucent_objects.begin(); it != _translucent_objects.end(); ++it) {
        (*it)->render(_camera, _dir_lights, dest_fbo, _bkg_color_tex, _bkg_depth_tex, _skybox_texture);
    }

    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Simulate fluid
    if (dt > 0.0f) {
        auto fluid = std::dynamic_pointer_cast<Fluid>(get_object("fluid"));
        if (fluid) {
            fluid->simulate();
        }
    }

    // Update the physics of the rigid bodies
    _bt_world->stepSimulation(dt, 0);
}

void Scene::_initialize_fbo(int viewport_width, 
                            int viewport_height,
                            GLuint& fbo,
                            GLuint& color_tex,
                            GLuint& depth_tex) {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glGenFramebuffers(1, &fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Tell opengl that we are not reading any color
    gl.glReadBuffer(GL_NONE);
    gl.glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // Initialize background color texture
    gl.glGenTextures(1, &color_tex);
    gl.glBindTexture(GL_TEXTURE_2D, color_tex);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gl.glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_RGB32F,
                    viewport_width,
                    viewport_height,
                    0,
                    GL_RGB,
                    GL_FLOAT,
                    0);

    gl.glFramebufferTexture(GL_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT0,
                            color_tex,
                            0);

    gl.glGenTextures(1, &depth_tex);
    gl.glBindTexture(GL_TEXTURE_2D, depth_tex);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gl.glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_DEPTH_COMPONENT24,
                    viewport_width,
                    viewport_height,
                    0,
                    GL_DEPTH_COMPONENT,
                    GL_FLOAT,
                    0);

    gl.glFramebufferTexture(GL_FRAMEBUFFER,
                            GL_DEPTH_ATTACHMENT,
                            depth_tex,
                            0);

    // Check FBO status
    auto status = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        throw RunTimeException("Scene framebuffer initializaction failed, CANNOT use this FBO");
    }

    // Unbind buffer
    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::_intialize_skybox() {
    GLfloat vertices[] = {
      -10.0f,  10.0f, -10.0f,
      -10.0f, -10.0f, -10.0f,
       10.0f, -10.0f, -10.0f,
       10.0f, -10.0f, -10.0f,
       10.0f,  10.0f, -10.0f,
      -10.0f,  10.0f, -10.0f,
      
      -10.0f, -10.0f,  10.0f,
      -10.0f, -10.0f, -10.0f,
      -10.0f,  10.0f, -10.0f,
      -10.0f,  10.0f, -10.0f,
      -10.0f,  10.0f,  10.0f,
      -10.0f, -10.0f,  10.0f,
      
       10.0f, -10.0f, -10.0f,
       10.0f, -10.0f,  10.0f,
       10.0f,  10.0f,  10.0f,
       10.0f,  10.0f,  10.0f,
       10.0f,  10.0f, -10.0f,
       10.0f, -10.0f, -10.0f,
       
      -10.0f, -10.0f,  10.0f,
      -10.0f,  10.0f,  10.0f,
       10.0f,  10.0f,  10.0f,
       10.0f,  10.0f,  10.0f,
       10.0f, -10.0f,  10.0f,
      -10.0f, -10.0f,  10.0f,
      
      -10.0f,  10.0f, -10.0f,
       10.0f,  10.0f, -10.0f,
       10.0f,  10.0f,  10.0f,
       10.0f,  10.0f,  10.0f,
      -10.0f,  10.0f,  10.0f,
      -10.0f,  10.0f, -10.0f,
      
      -10.0f, -10.0f, -10.0f,
      -10.0f, -10.0f,  10.0f,
       10.0f, -10.0f, -10.0f,
       10.0f, -10.0f, -10.0f,
      -10.0f, -10.0f,  10.0f,
       10.0f, -10.0f,  10.0f
    };
    
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glGenVertexArrays(1, &_skybox_vao);
    gl.glBindVertexArray(_skybox_vao);
    
    // Set up box vertices
    gl.glGenBuffers(1, &_skybox_vertices_vbo);
    gl.glBindBuffer(GL_ARRAY_BUFFER, _skybox_vertices_vbo);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    36 * sizeof(GLfloat) * 3,
                    vertices,
                    GL_STATIC_DRAW);

    // Load shader program
    _shader = create_shader_program("shaders/skybox.vert",
                                    "shaders/skybox.frag");
    
    auto vertex_loc = _shader->attributeLocation("v_position");
    _shader->enableAttributeArray(vertex_loc);
    gl.glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Initialize the skybox texture
    gl.glEnable(GL_TEXTURE_CUBE_MAP);
    gl.glGenTextures(1, &_skybox_texture);
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_CUBE_MAP, _skybox_texture);

    string texture_files[] = {
        "posx.jpg",
        "negx.jpg",
        "posy.jpg",
        "negy.jpg",
        "posz.jpg",
        "negz.jpg"
    };
    string textures_dir = "textures/skansen/";

    for (auto i = 0; i < 6; ++i) {
        QImage img((textures_dir + texture_files[i]).c_str());
        gl.glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0,
                        GL_RGBA,
                        img.width(),
                        img.height(),
                        0,
                        GL_BGRA,
                        GL_UNSIGNED_BYTE,
                        img.bits());
    }

    gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl.glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl.glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Scene::_render_skybox() {
    auto& gl = OpenGLFunctions::getFunctions();

    // Render the skybox
    gl.glBindVertexArray(_skybox_vao);

    // Bind the shader to be the used program
    _shader->bind();

    // Load the projection and modelview matrix
    QMatrix4x4 transformation;
    transformation.setToIdentity();
    _shader->setUniformValue("mv_matrix", _camera.rotation());
    _shader->setUniformValue("pr_matrix", _camera.projection());
    _shader->setUniformValue("skybox", 0);

    // Render the skybox
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_CUBE_MAP, _skybox_texture);
    gl.glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Scene::_intialize_bullets() {
    _bt_broadphase = new btDbvtBroadphase();
    _bt_collision_configuration = new btDefaultCollisionConfiguration();
    _bt_dispatcher = new btCollisionDispatcher(_bt_collision_configuration);
    _bt_solver = new btSequentialImpulseConstraintSolver();

    _bt_world = new btDiscreteDynamicsWorld(_bt_dispatcher,
                                            _bt_broadphase,
                                            _bt_solver,
                                            _bt_collision_configuration);

    _bt_world->setGravity(btVector3(0, -9.8, 0));
}

void Scene::add_directional_light(const DirectionalLight& light) {
    _dir_lights.push_back(light);
}

unique_ptr<Scene> Scene::load_scene(const std::string& filename, 
                                    int viewport_width, 
                                    int viewport_height) {
    auto scene = make_unique<Scene>(viewport_width, viewport_height);
    
    ifstream f(filename.c_str(), ifstream::in);
    if (!f.good()) {
        throw RunTimeException("Could not parse scene file: " + filename);
    }


    string f_str((istreambuf_iterator<char>(f)),
                  istreambuf_iterator<char>());

    string parse_errors;
    auto scene_config = json11::Json::parse(f_str, parse_errors);

    auto lights = scene_config["lights"].array_items();
    for (auto& l : lights) {
        DirectionalLight dir_light;
        dir_light.direction = QVector3D(
            l["direction"][0].number_value(), 
            l["direction"][1].number_value(), 
            l["direction"][2].number_value()
        );
        dir_light.ambient_color = QColor(
            l["ambient_color"][0].int_value(), 
            l["ambient_color"][1].int_value(), 
            l["ambient_color"][2].int_value()
        );
        
        dir_light.diffuse_color = QColor(
            l["diffuse_color"][0].int_value(), 
            l["diffuse_color"][1].int_value(), 
            l["diffuse_color"][2].int_value()
        );
        
        dir_light.specular_color = QColor(
            l["specular_color"][0].int_value(), 
            l["specular_color"][1].int_value(), 
            l["specular_color"][2].int_value()
        );
        
        scene->add_directional_light(dir_light);
    }
    
    auto fluid = make_shared<Fluid>(viewport_width, 
                                    viewport_height,
                                    Settings::physics(),
                                    Settings::simulation(),
                                    Settings::graphics());
    scene->add_object("fluid", fluid, true);
    
    // Set up simulation container
    auto container_size = scene_config["container"];
    float width = container_size["width"].number_value();
    float height = container_size["height"].number_value();
    float depth = container_size["depth"].number_value();
    auto container = make_shared<FishTank>(width, height, depth, 0.0);
    scene->add_rigid_body("container", container);
    fluid->set_rect_limits(width, height, depth);

    auto fluid_volumes = scene_config["fluid_volumes"].array_items();
    for (auto& v : fluid_volumes) {
        if (v["type"] == "box") {
            auto size = btVector3(v["size"][0].number_value(), v["size"][1].number_value(), v["size"][2].number_value());
            auto center = btVector3(v["center"][0].number_value(), v["center"][1].number_value(), v["center"][2].number_value());
            auto vol = make_shared<BoxVolume>(size, center);
            fluid->add_volume(vol);
        }
    }

    auto rigid_bodies = scene_config["rigid_bodies"].array_items();
    for (auto& r : rigid_bodies) {
        auto type = r["type"];
        auto name = r["name"].string_value();
        auto mass = r["mass"].number_value();
        btVector3 center(0, 0, 0);
        btQuaternion rotation(0, 0, 0, 1);
        shared_ptr<RigidBody> body = nullptr;
               
        if (type == "cube") {
            center = btVector3(
                r["center"][0].number_value(),
                r["center"][1].number_value(),
                r["center"][2].number_value()
            );
            rotation = btQuaternion(
                r["rotation"][0].number_value(),
                r["rotation"][1].number_value(),
                r["rotation"][2].number_value(),
                r["rotation"][3].number_value()
            );
            body = make_shared<Cube>(r["size"].number_value(),
                                     mass,
                                     center,
                                     rotation);
        }
        else if (type == "sphere") {
            center = btVector3(
                r["center"][0].number_value(),
                r["center"][1].number_value(),
                r["center"][2].number_value()
            );
            body = make_shared<Sphere>(r["size"].number_value(),
                                       mass,
                                       center);
        }
        else if (type == "model") {
            center = btVector3(
                r["center"][0].number_value(),
                r["center"][1].number_value(),
                r["center"][2].number_value()
            );
            rotation = btQuaternion(
                r["rotation"][0].number_value(),
                r["rotation"][1].number_value(),
                r["rotation"][2].number_value(),
                r["rotation"][3].number_value()
            );
            auto obj_filename = r["obj_filename"].string_value();
            body = make_shared<Model>(obj_filename,
                                      mass,
                                      center,
                                      rotation);
        }
        else if (type == "wall") {
            center = btVector3(
                r["center"][0].number_value(),
                r["center"][1].number_value(),
                r["center"][2].number_value()
            );
            rotation = btQuaternion(
                r["rotation"][0].number_value(),
                r["rotation"][1].number_value(),
                r["rotation"][2].number_value(),
                r["rotation"][3].number_value()
            );
            float width = r["size"][0].number_value();
            float height = r["size"][1].number_value();
            body = make_shared<Wall>(width,
                                     height,
                                     mass,
                                     center,
                                     rotation);
        }

        if (body) {
            scene->add_rigid_body(name, body);
            fluid->add_boundary(body, name, mass > 0.0f);
        }
    }

    return scene;
}

void Scene::reset(SimulationSettings s_settings,
                  PhysicsSettings p_settings,
                  GraphicsSettings g_settings) {
    auto fluid = std::dynamic_pointer_cast<Fluid>(get_object("fluid"));
    if (fluid) {
        fluid->reset(s_settings, p_settings, g_settings);
    }

    // Reset position and rotation of every object
    for (auto& key_val : _scene_objects) {
        auto id = key_val.first;
        auto p = _initial_positions[id];
        auto q = _initial_rotations[id];
        auto obj = key_val.second;
        obj->set_position(p);
        obj->set_rotation(q);
    }
}

void Scene::_copy_fbo(GLuint from_fbo, GLuint dest_fbo) {
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glBindFramebuffer(GL_READ_FRAMEBUFFER, from_fbo);
    gl.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_fbo);
    gl.glReadBuffer(GL_COLOR_ATTACHMENT0);
    gl.glBlitFramebuffer(0, 0, _viewport_w, _viewport_h,
                         0, 0, _viewport_w, _viewport_h,
                         GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                         GL_NEAREST);
}