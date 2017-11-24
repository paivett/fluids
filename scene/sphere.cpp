#include "sphere.h"
#include "opengl/glutils.h"
#include <iostream>
#include <sstream>

using namespace std;

Sphere::Sphere(float radius,
               float mass,
               const btVector3& pos)
    : RigidBody(mass, pos, btQuaternion(0, 0, 0, 1)),
      _radius(radius),
      _sphere_mesh(new IcoSphereMesh(radius, 3)) {

    // Initialize mesh
    auto& gl = OpenGLFunctions::getFunctions();

    // Load shader program
    _shader = create_shader_program("shaders/phong.vert",
                                    "shaders/phong.frag");

    gl.glGenVertexArrays(1, &_vao);
    gl.glBindVertexArray(_vao);
    gl.glGenBuffers(3, _vbo_ids);

    // Set up box vertices
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[0]);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    _sphere_mesh->vertices_count() * 3 * sizeof(float),
                    _sphere_mesh->vertices(),
                    GL_STATIC_DRAW);

    auto vertex_loc = _shader->attributeLocation("vertex_coord");
    _shader->enableAttributeArray(vertex_loc);
    gl.glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Set up box normals
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[1]);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    _sphere_mesh->vertices_count() * 3 * sizeof(float),
                    _sphere_mesh->normals(),
                    GL_STATIC_DRAW);

    auto normal_loc = _shader->attributeLocation("vertex_normal");
    _shader->enableAttributeArray(normal_loc);
    gl.glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Set up box indices
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[2]);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    _sphere_mesh->indices_count() * sizeof(mesh_index),
                    _sphere_mesh->mesh_indices(),
                    GL_STATIC_DRAW);   

    //Default color
    set_material(get_random_material());

    // Initialize bullet physics
    _init_physics();
}

Sphere::~Sphere() {
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glDeleteBuffers(3, _vbo_ids);
}

void Sphere::render(const Camera& camera, 
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

    // Render the sphere
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[2]);
    gl.glDrawElements(GL_TRIANGLES, _sphere_mesh->indices_count(), GL_UNSIGNED_SHORT, 0);
}

float Sphere::radius() const {
    return _radius;
}

void Sphere::_init_physics() {
    _bt_collision_shape = new btSphereShape(_radius);
    _motion_state = new btDefaultMotionState(_transform);
    _bt_collision_shape->calculateLocalInertia(_mass, _local_inertia);
    _bt_rigid_body = new btRigidBody(_mass, _motion_state, _bt_collision_shape, _local_inertia);
}

vector<btVector3> Sphere::surface_sampling(float particle_spacing) const {
    vector<btVector3> particles;

    auto vertices = _sphere_mesh->vertices();
    auto indices = _sphere_mesh->mesh_indices();
    float area_particle = M_PI * pow(particle_spacing, 2);

    for (size_t i=0; i<_sphere_mesh->indices_count(); i+=3) {
        // vec3 a = vertices[indices[i]];
        // vec3 b = vertices[indices[i+1]];
        // vec3 c = vertices[indices[i+2]];
        btVector3 a(vertices[indices[i]], vertices[indices[i]+1], vertices[indices[i]+2]);
        btVector3 b(vertices[indices[i+1]], vertices[indices[i+1]+1], vertices[indices[i+1]+2]);
        btVector3 c(vertices[indices[i+2]], vertices[indices[i+2]+1], vertices[indices[i+2]+2]);

        auto ab = a-b;
        auto ac = a-c;

        float area_triangle = 0.5f * sqrt(pow(ab.y() * ac.z() - ab.z() * ac.y(), 2) + pow(ab.z() * ac.x() - ab.x() * ac.z(), 2) + pow(ab.x() * ac.y() - ab.x() * ac.y(), 2));
        int n = 2*ceil(area_triangle / area_particle);

        default_random_engine generator;
        uniform_real_distribution<float> rnd(0, 1);
        for (int k=0; k<n; k++) {
            // http://math.stackexchange.com/questions/18686/uniform-random-point-in-triangle
            float r1 = sqrt(rnd(generator));
            float r2 = rnd(generator);
            
            btVector3 p = (1.0f - r1)*a + (r1*(1.0f - r2))*b + (r2*r1)*c;
            particles.push_back(p);
        }
    }

    for (size_t i=0; i<_sphere_mesh->vertices_count(); ++i) {
        btVector3 p(vertices[3*i], vertices[3*i+1], vertices[3*i+2]);
        particles.push_back(p);
    }

    return particles;
}
