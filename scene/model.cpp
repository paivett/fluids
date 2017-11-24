#include "model.h"
#include "opengl/glutils.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"
#include <sstream>
#include <iostream>

#define VOXELIZER_IMPLEMENTATION
#include "external/voxelizer/voxelizer.h"

using namespace std;

Model::Model(const string& model_filename,
             float mass,
             const btVector3& pos,
             const btQuaternion& q)
    : RigidBody(mass, pos, q),
      _model_mesh(new ModelMesh(model_filename)) {
    
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
                    _model_mesh->vertices_count() * 3 * sizeof(float),
                    _model_mesh->normals(),
                    GL_STATIC_DRAW);

    auto normal_loc = _shader->attributeLocation("vertex_normal");
    _shader->enableAttributeArray(normal_loc);
    gl.glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Set up box vertices
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[0]);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    _model_mesh->vertices_count() * 3 * sizeof(float),
                    _model_mesh->vertices(),
                    GL_STATIC_DRAW);

    auto vertex_loc = _shader->attributeLocation("vertex_coord");
    _shader->enableAttributeArray(vertex_loc);
    gl.glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
  
    // Set up box indices
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[2]);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    _model_mesh->indices_count() * sizeof(mesh_index),
                    _model_mesh->mesh_indices(),
                    GL_STATIC_DRAW);

    set_material(get_random_material());

    // Initialize bullet physics
    _init_physics();
}

Model::~Model() {
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glDeleteBuffers(3, _vbo_ids);
}

void Model::render(const Camera& camera, 
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

    // Render the model
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[2]);
    gl.glDrawElements(GL_TRIANGLES, _model_mesh->indices_count(), GL_UNSIGNED_SHORT, nullptr);
}

void Model::_init_physics() {
    // Extract vertices
    btScalar* bt_vertices = new btScalar[_model_mesh->vertices_count()*3];
    auto bt_indices = new int[_model_mesh->indices_count()];

    int faces_count = (int)_model_mesh->indices_count() / 3;
    for (int i=0; i<_model_mesh->vertices_count()*3; ++i) {
        bt_vertices[i] = _model_mesh->vertices()[i];
    }

    for (int i=0; i<_model_mesh->indices_count(); ++i) {
        bt_indices[i] = _model_mesh->mesh_indices()[i];
    }

    auto index_vertex_arrays = new btTriangleIndexVertexArray(faces_count,
                                                              bt_indices, 
                                                              3 * sizeof(int),
                                                              _model_mesh->vertices_count(),
                                                              bt_vertices, 
                                                              sizeof(btScalar) * 3);

    auto tmp_convex_shape = new btConvexTriangleMeshShape(index_vertex_arrays);

    // Create a hull approximation
    auto hull = new btShapeHull(tmp_convex_shape);
    auto margin = tmp_convex_shape->getMargin();
    hull->buildHull(margin);   
    auto convex_hull_shape = new btConvexHullShape();
    for(int i= 0; i < hull->numVertices(); i++ ) {
       convex_hull_shape->addPoint(hull->getVertexPointer()[i]);   
    }    
    _bt_collision_shape = convex_hull_shape;
    _motion_state = new btDefaultMotionState(_transform);
    _bt_collision_shape->calculateLocalInertia(_mass, _local_inertia);
    _bt_rigid_body = new btRigidBody(_mass, _motion_state, _bt_collision_shape, _local_inertia);

    delete tmp_convex_shape;
    delete hull;
    delete bt_vertices;
    delete bt_indices;
    delete index_vertex_arrays;
}

vector<btVector3> Model::surface_sampling(float particle_spacing) const {
    vector<btVector3> sampling;
    vx_mesh_t* original_mesh;
    vx_mesh_t* voxelized_mesh;

    int vertex_count = _model_mesh->vertices_count();
    int index_count = _model_mesh->indices_count();

    original_mesh = vx_mesh_alloc(vertex_count, index_count);

    // Load current mesh vertices
    for (int i=0; i<vertex_count; ++i) {
        original_mesh->vertices[i].x = _model_mesh->vertices()[3*i + 0];
        original_mesh->vertices[i].y = _model_mesh->vertices()[3*i + 1];
        original_mesh->vertices[i].z = _model_mesh->vertices()[3*i + 2];
    }

    // Load current mesh indices
    for (int i=0; i<index_count; ++i) {
        original_mesh->indices[i] = _model_mesh->mesh_indices()[i];
    }

    // Precision factor to reduce "holes" artifact
    float precision = particle_spacing / 10;

    // Run voxelization
    voxelized_mesh = vx_voxelize(original_mesh, 
                                 particle_spacing, 
                                 particle_spacing, 
                                 particle_spacing, 
                                 precision);

    // Iterate over the generated vertices
    for (int i=0; i<voxelized_mesh->nvertices; ++i) {
        sampling.push_back(btVector3(
            voxelized_mesh->vertices[i].x,
            voxelized_mesh->vertices[i].y,
            voxelized_mesh->vertices[i].z
        ));
    }    

    delete original_mesh;
    delete voxelized_mesh;

    return sampling;
}