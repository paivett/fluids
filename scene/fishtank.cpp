#include "fishtank.h"
#include "opengl/glutils.h"

using namespace std;

FishTank::FishTank(float w, float h, float d, float mass) :
RigidBody(mass, 0, 0 ,0),
_width(w),
_height(h),
_depth(d),
_box_mesh(new WireBoxMesh(w, h, d)) {
    // Initialize mesh
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glGenVertexArrays(1, &_vao);
    gl.glBindVertexArray(_vao);
    gl.glGenBuffers(2, _vbo_ids);

    // Set up box vertices
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[0]);
    gl.glBufferData(GL_ARRAY_BUFFER,
                    _box_mesh->vertices_count() * 3 * sizeof(float),
                    _box_mesh->vertices(),
                    GL_STATIC_DRAW);

    // Set up box indices
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[1]);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    _box_mesh->indices_count() * sizeof(mesh_index),
                    _box_mesh->mesh_indices(),
                    GL_STATIC_DRAW);

    // Load shader program
    _shader = create_shader_program("shaders/basic.vs.glsl",
                                    "shaders/basic.fs.glsl");

    int vertex_loc = _shader->attributeLocation("v_position");
    _shader->enableAttributeArray(vertex_loc);
    gl.glVertexAttribPointer(vertex_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Default color
    set_color(0.53, 0.2039, 0.0039);

    _init_physics();
}

FishTank::~FishTank() {
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glDeleteBuffers(2, _vbo_ids);
}

void FishTank::render(const Camera& camera,
                      const std::vector<DirectionalLight>& dir_lights,
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
    _qt_transformation.setToIdentity();
    //transformation.rotate(_rotation);
    _qt_transformation.translate(_transform.getOrigin().x(),
                                 _transform.getOrigin().y(),
                                 _transform.getOrigin().z());

    _shader->setUniformValue("mv_matrix", camera.transformation() * _qt_transformation);
    _shader->setUniformValue("pr_matrix", camera.projection());
    _shader->setUniformValue("color", _color);

    // Render grid
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo_ids[1]);
    gl.glDrawElements(GL_LINES, _box_mesh->indices_count(), GL_UNSIGNED_SHORT, 0);
}

float FishTank::width() const {
    return _width;
}

float FishTank::height() const {
    return _height;
}

float FishTank::depth() const {
    return _depth;
}

void FishTank::set_color(float r, float g, float b, float alpha) {
    _color.setRedF(r);
    _color.setGreenF(g);
    _color.setBlueF(b);
    _color.setAlphaF(alpha);
}

void FishTank::set_color(const QColor &c) {
    _color = c;
}

QColor FishTank::color() const {
    return _color;
}

vector<btVector3> FishTank::surface_sampling(float particle_spacing) const {
    vector<btVector3> particles;

    auto width = _width;
    auto height = _height;
    auto depth = _depth;
    
    // Rect tank base
    for (auto x = -width/2; x <= width/2; x += particle_spacing) {
        for (auto z = -depth/2; z <= depth/2; z += particle_spacing) {
            auto p1 = _transform(btVector3(x, -height/2, z));
            particles.push_back(p1);
        }    
    }

    // Walls
    float y0 = -height/2 + particle_spacing;
    for (auto y = y0; y <= height/2; y += particle_spacing) {
        for (auto x = -width/2; x <= width/2; x += particle_spacing) {
            auto p1 = _transform(btVector3(x, y, -depth/2));
            auto p2 = _transform(btVector3(x, y, depth/2));
            particles.push_back(p1);
            particles.push_back(p2);
        }
        for (auto z = -depth/2; z <= depth/2; z += particle_spacing) {
            auto p1 = _transform(btVector3(width/2, y, z));
            auto p2 = _transform(btVector3(-width/2, y, z));
            particles.push_back(p1);
            particles.push_back(p2);
        } 
    }

    return particles;
}

void FishTank::_init_physics() {
    btTriangleMesh* triangles = new btTriangleMesh();
    auto x = _width/2;
    auto y = _height/2;
    auto z = _depth/2;
    
    btVector3 p0(-x, -y, -z);
    btVector3 p1(-x, -y,  z);
    btVector3 p2(-x,  y,  z);
    btVector3 p3(-x,  y, -z);
    btVector3 p4( x,  y, -z);
    btVector3 p5( x, -y,  z);
    btVector3 p6( x, -y, -z);
    btVector3 p7( x,  y,  z);
    
    // Left
    triangles->addTriangle(p0, p1, p2);
    triangles->addTriangle(p0, p2, p3);
    
    // Bottom
    triangles->addTriangle(p0, p1, p5);
    triangles->addTriangle(p0, p6, p5);

    // Right
    triangles->addTriangle(p5, p7, p6);
    triangles->addTriangle(p7, p6, p4);
    
    // Front
    triangles->addTriangle(p1, p2, p5);
    triangles->addTriangle(p2, p5, p7);

    // Back
    triangles->addTriangle(p3, p4, p6);
    triangles->addTriangle(p6, p0, p3);

    _bt_collision_shape = new btBvhTriangleMeshShape(triangles, true, true);
    _motion_state = new btDefaultMotionState(_transform);
    _bt_collision_shape->calculateLocalInertia(_mass, _local_inertia);
    _bt_rigid_body = new btRigidBody(_mass, _motion_state, _bt_collision_shape, _local_inertia);
}