#include "cube.h"
#include "opengl/glutils.h"
#include <sstream>
#include <iostream>

using namespace std;

Cube::Cube(float side, 
           float mass,
           const btVector3& pos,
           const btQuaternion& q)
    : RigidBody(mass, pos, q),
      _side(side),
      _box_mesh(new BoxMesh(side, side, side)) {
    
    // Initialize mesh
    auto& gl = OpenGLFunctions::getFunctions();

    // Load shader program
    _shader = create_shader_program("shaders/phong.vert",
                                    "shaders/phong.frag");


    gl.glGenVertexArrays(1, &_vao);
    gl.glBindVertexArray(_vao);
    gl.glGenBuffers(3, _vbo_ids);

    // Set up box normals
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[1]);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    _box_mesh->vertices_count() * 3 * sizeof(float),
                    _box_mesh->normals(),
                    GL_STATIC_DRAW);

    auto normal_loc = _shader->attributeLocation("vertex_normal");
    _shader->enableAttributeArray(normal_loc);
    gl.glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Set up box vertices
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[0]);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    _box_mesh->vertices_count() * 3 * sizeof(float),
                    _box_mesh->vertices(),
                    GL_STATIC_DRAW);

    auto vertex_loc = _shader->attributeLocation("vertex_coord");
    _shader->enableAttributeArray(vertex_loc);
    gl.glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
    // Set up box indices
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[2]);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    _box_mesh->indices_count() * sizeof(mesh_index),
                    _box_mesh->mesh_indices(),
                    GL_STATIC_DRAW);    

    set_material(get_random_material());

    // Initialize bullet physics
    _init_physics();
}

Cube::~Cube() {
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glDeleteBuffers(3, _vbo_ids);
}

void Cube::render(const Camera& camera, 
                  const vector<DirectionalLight>& dir_lights, 
                  GLuint fbo, 
                  GLuint bkg_texture,
                  GLuint bkg_depth_texture,
                  GLuint cube_map_texture) {
    // Load VAO state
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glBindVertexArray(_vao);

    // Bind the shader to be the used program
    _shader->bind();

    // Load the projection and modelview matrix
    auto t = transform();
    auto q = t.getRotation();
    auto r_axis = q.getAxis();
    auto r_angle = q.getAngle();
    auto q_rot = QQuaternion::fromAxisAndAngle(r_axis.x(), r_axis.y(), r_axis.z(), r_angle * 180 / M_PI);
    _qt_transformation.setToIdentity();
    _qt_transformation.translate(t.getOrigin().x(),
                                 t.getOrigin().y(),
                                 t.getOrigin().z());
    _qt_transformation.rotate(q_rot);

    auto v_matrix = camera.transformation();
    _shader->setUniformValue("m_matrix", _qt_transformation);
    _shader->setUniformValue("v_matrix", v_matrix);
    _shader->setUniformValue("inv_v_matrix", v_matrix.inverted());
    _shader->setUniformValue("p_matrix", camera.projection());
    _shader->setUniformValue("normal_matrix", _qt_transformation.normalMatrix());
    
    // Configure directional lights
    for (auto i=0; i < dir_lights.size(); ++i) {
        stringstream ss;
        ss << "dir_light[" << i << "]";
        string base = ss.str();
        _shader->setUniformValue((base + ".direction").c_str(), dir_lights[i].direction);
        _shader->setUniformValue((base + ".diffuse").c_str(), dir_lights[i].diffuse_color);
        _shader->setUniformValue((base + ".ambient").c_str(), dir_lights[i].ambient_color);
        _shader->setUniformValue((base + ".specular").c_str(), dir_lights[i].specular_color);
    }

    // Set the number of directional lights
    _shader->setUniformValue("dir_light_count", (GLint)dir_lights.size());
  
    // Material configuration
    _shader->setUniformValue("material_ambient", _material.ambient_color);
    _shader->setUniformValue("material_diffuse", _material.diffuse_color);
    _shader->setUniformValue("material_specular", _material.specular_color);
    _shader->setUniformValue("material_shininess", _material.shininess);

    // Render the cube
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[2]);
    gl.glDrawElements(GL_TRIANGLES, _box_mesh->indices_count(), GL_UNSIGNED_SHORT, nullptr);
}

float Cube::side() const {
    return _side;
}

void Cube::_init_physics() {
    _bt_collision_shape = new btBoxShape(btVector3(_side/2, _side/2, _side/2));
    _motion_state = new btDefaultMotionState(_transform);
    _bt_collision_shape->calculateLocalInertia(_mass, _local_inertia);
    _bt_rigid_body = new btRigidBody(_mass, _motion_state, _bt_collision_shape, _local_inertia);
}

vector<btVector3> Cube::surface_sampling(float particle_spacing) const {
    vector<btVector3> particles;
    
    auto width = _side - particle_spacing;
    auto height = _side - particle_spacing;
    auto depth = _side - particle_spacing;
    
    // Rect tank base
    for (auto x = -width/2; x <= width/2; x += particle_spacing) {
        for (auto z = -depth/2; z <= depth/2; z += particle_spacing) {
            auto p1 = (btVector3(x, -height/2, z));
            auto p2 = (btVector3(x, height/2, z));
            particles.push_back(p1);
            particles.push_back(p2);
        }    
    }

    // Walls
    float y0 = -height/2 + particle_spacing;
    for (auto y = y0; y <= height/2; y += particle_spacing) {
        for (auto x = -width/2; x <= width/2; x += particle_spacing) {
            auto p1 = (btVector3(x, y, -depth/2));
            auto p2 = (btVector3(x, y, depth/2));
            particles.push_back(p1);
            particles.push_back(p2);
        }
        for (auto z = -depth/2; z <= depth/2; z += particle_spacing) {
            auto p1 = (btVector3(width/2, y, z));
            auto p2 = (btVector3(-width/2, y, z));
            particles.push_back(p1);
            particles.push_back(p2);
        } 
    }

    return particles;
}