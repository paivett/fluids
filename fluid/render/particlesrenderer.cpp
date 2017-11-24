#include "particlesrenderer.h"
#include "runtimeexception.h"
#include <opencl/clenvironment.h>
#include <opengl/openglfunctions.h>
#include <CL/cl_gl.h>
#include <sstream>

using namespace std;

ParticlesRenderer::ParticlesRenderer(int viewport_width,
                                     int viewport_height,
                                     int particle_count,
                                     GLuint vbo_particles) :
_vbo_particles(vbo_particles),
_particle_count(particle_count),
_viewport_w(viewport_width),
_viewport_h(viewport_height) {
    auto& gl = OpenGLFunctions::getFunctions();

    _shader = create_shader_program("shaders/particles.vert",
                                    "shaders/particles.frag");

    _shader->bind();

    // Create a VAO for this stage
    gl.glGenVertexArrays(1, &_vao);
    gl.glBindVertexArray(_vao);

    // Bind the VBO to the shader
    auto particle_pos_loc = _shader->attributeLocation("particle_pos");
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_particles);
    gl.glVertexAttribPointer(particle_pos_loc,
                             4,
                             GL_FLOAT,
                             GL_TRUE,
                             0,
                             NULL);
    _shader->enableAttributeArray(particle_pos_loc);
}

void ParticlesRenderer::render(const Camera& camera,
                               const vector<DirectionalLight>& dir_lights,
                               const QMatrix4x4& mv_matrix,
                               GLuint dest_fbo,
                               GLuint background_texture,
                               GLuint bkg_depth_texture,
                               GLuint cube_map_texture) {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);

    _shader->bind();

    // Configure shader uniforms
    _shader->setUniformValue("mv_matrix", mv_matrix);
    _shader->setUniformValue("pr_matrix", camera.projection());

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

    // Get perspective info to calculate point sprite size
    auto perspective = camera.perspective();
    _shader->setUniformValue("point_radius", 0.125f * 0.5f);
    _shader->setUniformValue("point_scale", perspective.width * tanf(perspective.fov * (0.5f * 3.1415926535f/180.0f)));
    _shader->setUniformValue("near_plane", perspective.nearPlane);
    _shader->setUniformValue("far_plane", perspective.farPlane);

    // Render particles as point sprites
    gl.glEnable(GL_DEPTH_TEST);
    gl.glEnable(GL_PROGRAM_POINT_SIZE);

    // Bind vertex array and draw it
    gl.glBindVertexArray(_vao);
    gl.glDrawArrays(GL_POINTS, 0, _particle_count);

    gl.glDisable(GL_PROGRAM_POINT_SIZE);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ParticlesRenderer::reset(int particle_count) {
    _particle_count = particle_count;
}

ParticlesRenderer::~ParticlesRenderer() {
    auto& gl = OpenGLFunctions::getFunctions();
    gl.glDeleteVertexArrays(1, &_vao);
}