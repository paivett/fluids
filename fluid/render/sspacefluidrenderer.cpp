#include "sspacefluidrenderer.h"
#include "runtimeexception.h"
#include <opencl/clenvironment.h>
#include <CL/cl_gl.h>
#include <iostream>
using namespace std;

#define RESOLUTION_FACTOR       (1.00f)
#define ADJUST_RESOLUTION(X)    (X/RESOLUTION_FACTOR)

SSpaceFluidRenderer::SSpaceFluidRenderer(int viewport_width,
                                         int viewport_height,
                                         int filter_iterations,
                                         int particle_count,
                                         GLuint vbo_particles) :
_vbo_particles(vbo_particles),
_particle_count(particle_count),
_depth_filter(ADJUST_RESOLUTION(viewport_width), ADJUST_RESOLUTION(viewport_height)),
_filter_iterations(filter_iterations),
_viewport_w(viewport_width),
_viewport_h(viewport_height) {
    // First thing, initialize the viewport quad
    _init_viewport_quad();

    // Initialize all the stages of the screen space fluid rendering, including
    // all the fbo's, textures, and shaders
    int h = ADJUST_RESOLUTION(viewport_height);
    int w = ADJUST_RESOLUTION(viewport_width);
    _init_depth_stage(w, h);
    _init_blur_stage(w, h);
    _init_normals_stage(w, h);
    _init_final_stage();
}

void SSpaceFluidRenderer::render(const Camera& camera,
                                 const vector<DirectionalLight>& dir_lights,
                                 const QMatrix4x4& mv_matrix,
                                 GLuint dest_fbo,
                                 GLuint background_texture,
                                 GLuint bkg_depth_texture,
                                 GLuint cube_map_texture) {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glViewport(0, 0, ADJUST_RESOLUTION(_viewport_w), ADJUST_RESOLUTION(_viewport_h));
    _render_depth_stage(mv_matrix, camera, bkg_depth_texture);
    _render_blur_stage(mv_matrix, camera);
    _render_normals_stage(camera);
    
    gl.glViewport(0, 0, _viewport_w, _viewport_h);
    _render_final_stage(mv_matrix,
                        camera,
                        dest_fbo,
                        background_texture,
                        cube_map_texture);
}

void SSpaceFluidRenderer::_init_viewport_quad() {
    auto& gl = OpenGLFunctions::getFunctions();
    GLfloat quad_data[] = {
        -1, 1, 0, 1,
        -1, -1, 0, 0,
        1, -1, 1, 0,
        1, 1, 1, 1
    };

    GLushort indices[] = {0, 1, 2, 0, 2, 3};

    // Allocate vbos for data
    gl.glGenBuffers(1, &_quad_data_vbo);
    gl.glGenBuffers(1, &_quad_indices_vbo);

    // Upload vertex data
    gl.glBindBuffer(GL_ARRAY_BUFFER, _quad_data_vbo);
    gl.glBufferData(GL_ARRAY_BUFFER, 
                    sizeof(quad_data), 
                    quad_data, 
                    GL_STATIC_DRAW);

    // indices
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_indices_vbo);
    gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    sizeof(indices),
                    indices,
                    GL_STATIC_DRAW);

    // Unbind VAO for safety return
    gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    gl.glBindVertexArray(0);
}

void SSpaceFluidRenderer::_init_depth_stage(int width, int height) {
    auto& gl = OpenGLFunctions::getFunctions();

    // Create a VAO for this stage
    gl.glGenVertexArrays(1, &_depth_stage_vao);
    gl.glBindVertexArray(_depth_stage_vao);

    // Load and bind the shader program for this stage
    _depth_stage_shader = create_shader_program("shaders/sspace_fluid_rendering/depth_stage.vert",
                                                "shaders/sspace_fluid_rendering/depth_stage.frag");

    _depth_stage_shader->bind();

    // Initialize some uniform values
    _depth_stage_shader->setUniformValue("viewport_res_factor", RESOLUTION_FACTOR);
    _depth_stage_shader->setUniformValue("bkg_width", width*RESOLUTION_FACTOR);
    _depth_stage_shader->setUniformValue("bkg_height", height*RESOLUTION_FACTOR);
    _depth_stage_shader->setUniformValue("bkg_depth_texture", 0);

    // Bind the VBO to the shader
    auto particle_pos_loc = _depth_stage_shader->attributeLocation("particle_pos");
    gl.glBindBuffer(GL_ARRAY_BUFFER, _vbo_particles);
    gl.glVertexAttribPointer(particle_pos_loc,
                             4,
                             GL_FLOAT,
                             GL_TRUE,
                             0,
                             NULL);
    _depth_stage_shader->enableAttributeArray(particle_pos_loc);

    // Create the texture to store the depth values
    gl.glGenTextures(1, &_depth_texture);

    // Bind it, and initialize it
    gl.glBindTexture(GL_TEXTURE_2D, _depth_texture);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    gl.glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_R32F,
                    width,
                    height,
                    0,
                    GL_RED,
                    GL_FLOAT,
                    0);

    // Create a opencl 2D image to have shared access to the texture
    cl_int err;
    _depth_images[0] = clCreateFromGLTexture2D(CLEnvironment::context(),
                                           CL_MEM_READ_WRITE,
                                           GL_TEXTURE_2D,
                                           0,
                                           _depth_texture,
                                           &err);
    CLError::check(err);

    // Create the framebuffer and bind it for configuration
    gl.glGenFramebuffers(1, &_depth_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, _depth_fbo);

    // Tell opengl that we are not reading or writing any color for this buffer
    gl.glReadBuffer(GL_NONE);
    gl.glDrawBuffer(GL_COLOR_ATTACHMENT0);

    gl.glBindTexture(GL_TEXTURE_2D, _depth_texture);
    gl.glFramebufferTexture(GL_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT0,
                            _depth_texture,
                            0);

    // Create and attach depth buffer
    gl.glGenRenderbuffers(1, &_depth_fbo_depth_buffer);
    gl.glBindRenderbuffer(GL_RENDERBUFFER, _depth_fbo_depth_buffer);
    gl.glRenderbufferStorage(GL_RENDERBUFFER,
                             GL_DEPTH_COMPONENT,
                             width,
                             height);
    gl.glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                 GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER,
                                 _depth_fbo_depth_buffer);

    // Check FBO status
    auto status = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        throw RunTimeException("Depth stage framebuffer initializaction failed, CANNOT use this FBO");
    }

    // Unbind texture and fbo for safety return
    gl.glBindTexture(GL_TEXTURE_2D, 0);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSpaceFluidRenderer::_init_blur_stage(int width, int height) {
    auto& gl = OpenGLFunctions::getFunctions();

    // Create the texture to store the blurred depth values
    gl.glGenTextures(1, &_blurred_depth_texture);

    // Bind it, and initialize it
    gl.glBindTexture(GL_TEXTURE_2D, _blurred_depth_texture);

    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    gl.glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_R32F,
                    width,
                    height,
                    0,
                    GL_RED,
                    GL_FLOAT,
                    0);

    // Create a opencl 2D image to have shared access to the texture
    cl_int err;
    _depth_images[1] = clCreateFromGLTexture2D(CLEnvironment::context(),
                                           CL_MEM_READ_WRITE,
                                           GL_TEXTURE_2D,
                                           0,
                                           _blurred_depth_texture,
                                           &err);
    CLError::check(err);

    // Unbind texture for safety return
    gl.glBindTexture(GL_TEXTURE_2D, 0);
}

void SSpaceFluidRenderer::_init_normals_stage(int width, int height) {
    auto& gl = OpenGLFunctions::getFunctions();

    // Create a VAO for this stage
    gl.glGenVertexArrays(1, &_normals_stage_vao);
    gl.glBindVertexArray(_normals_stage_vao);

    // Load and bind the shader program for this stage
    _normals_stage_shader = create_shader_program("shaders/sspace_fluid_rendering/normals_stage.vert",
                                                  "shaders/sspace_fluid_rendering/normals_stage.frag");

    _normals_stage_shader->bind();

    // Configure uniform sampler for depth texture
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE0, _blurred_depth_texture);
    _normals_stage_shader->setUniformValue("depth_texture", 0);

    // Configure the inputs of the shader
    auto v_pos_location = _normals_stage_shader->attributeLocation("quad_vertex_pos");
    auto t_coord_location = _normals_stage_shader->attributeLocation("quad_text_coord");

    _normals_stage_shader->enableAttributeArray(v_pos_location);
    _normals_stage_shader->enableAttributeArray(t_coord_location);

    gl.glBindBuffer(GL_ARRAY_BUFFER, _quad_data_vbo);
    gl.glVertexAttribPointer(v_pos_location,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             4 * sizeof(GLfloat), // 3 for position, 2 for vertex coord
                             0);

    gl.glVertexAttribPointer(t_coord_location,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             4 * sizeof(GLfloat), // 3 for position, 2 for vertex coord
                             (GLvoid*)(2 * sizeof(GLfloat)));

    // Create the texture to store the normals values
    gl.glGenTextures(1, &_normals_texture);

    // Bind it, and initialize it
    gl.glBindTexture(GL_TEXTURE_2D, _normals_texture);

    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gl.glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_RGB32F,
                    width,
                    height,
                    0,
                    GL_RGBA,
                    GL_FLOAT,
                    0);

    // Create the framebuffer and bind it for configuration
    gl.glGenFramebuffers(1, &_normals_fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, _normals_fbo);

    gl.glReadBuffer(GL_NONE);
    gl.glDrawBuffer(GL_COLOR_ATTACHMENT0);
    gl.glBindTexture(GL_TEXTURE_2D, _normals_texture);
    gl.glFramebufferTexture(GL_FRAMEBUFFER,
                            GL_COLOR_ATTACHMENT0,
                            _normals_texture,
                            0);

    // Check FBO status
    auto status = gl.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
        throw RunTimeException("Normals stage framebuffer initializaction failed, CANNOT use this FBO");
    }

    // Unbind texture and fbo for safety return
    gl.glBindTexture(GL_TEXTURE_2D, 0);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSpaceFluidRenderer::_init_final_stage() {
    auto& gl = OpenGLFunctions::getFunctions();

    // Firt, initialize the program for this stage
    _final_stage_shader = create_shader_program("shaders/sspace_fluid_rendering/final_stage.vert",
                                                "shaders/sspace_fluid_rendering/final_stage.frag");

    _final_stage_shader->bind();

    _final_stage_shader->setUniformValue("blurred_depth_texture", 0);
    _final_stage_shader->setUniformValue("normals_texture", 1);
    _final_stage_shader->setUniformValue("background_texture", 2);
    _final_stage_shader->setUniformValue("cube_map_texture", 3);

    // Allocate vertex array
    gl.glGenVertexArrays(1, &_final_stage_vao);
    gl.glBindVertexArray(_final_stage_vao);

    auto v_pos_location = _final_stage_shader->attributeLocation("quad_vertex_pos");
    auto t_coord_location = _final_stage_shader->attributeLocation("quad_text_coord");

    _final_stage_shader->enableAttributeArray(v_pos_location);
    _final_stage_shader->enableAttributeArray(t_coord_location);

    gl.glBindBuffer(GL_ARRAY_BUFFER, _quad_data_vbo);
    gl.glVertexAttribPointer(v_pos_location,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             4 * sizeof(GLfloat), // 3 for position, 2 for vertex coord
                             0);

    gl.glVertexAttribPointer(t_coord_location,
                             2,
                             GL_FLOAT,
                             GL_FALSE,
                             4 * sizeof(GLfloat), // 3 for position, 2 for vertex coord
                             (GLvoid*)(2 * sizeof(GLfloat)));

    gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
    gl.glBindVertexArray(0);
}

void SSpaceFluidRenderer::_render_depth_stage(const QMatrix4x4& mv_matrix, 
                                              const Camera& camera,
                                              GLuint bkg_depth_texture) {
    auto& gl = OpenGLFunctions::getFunctions();
    _depth_stage_shader->bind();

    // Configure shader uniforms
    _depth_stage_shader->setUniformValue("mv_matrix", mv_matrix);
    _depth_stage_shader->setUniformValue("pr_matrix", camera.projection());

    // Get perspective info to calculate point sprite size
    auto perspective = camera.perspective();
    _depth_stage_shader->setUniformValue("point_radius", 0.125f * 0.5f);
    _depth_stage_shader->setUniformValue("point_scale", perspective.width * tanf(perspective.fov * (0.5f * 3.1415926535f/180.0f)));  

    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, bkg_depth_texture);

    // Bind depth fbo
    gl.glBindFramebuffer(GL_FRAMEBUFFER, _depth_fbo);
    gl.glEnable(GL_DEPTH_TEST);
    gl.glClearColor(0, 0, 0, 0);
    gl.glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // Render particles as point sprites
    gl.glEnable(GL_PROGRAM_POINT_SIZE);

    // Bind vertex array and draw it
    gl.glBindVertexArray(_depth_stage_vao);
    gl.glDrawArrays(GL_POINTS, 0, _particle_count);

    gl.glDisable(GL_PROGRAM_POINT_SIZE);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind fbo
}

void SSpaceFluidRenderer::_render_blur_stage(const QMatrix4x4& mv_matrix, const Camera& camera) {
    clEnqueueAcquireGLObjects(CLEnvironment::queue(),
                              2,
                              _depth_images,
                              0,
                              NULL,
                              NULL);

    for (int i=0; i < _filter_iterations; ++i) {
        _depth_filter.filter(_depth_images[0], _depth_images[1]);
        _depth_filter.filter(_depth_images[1], _depth_images[0]);
    }
    
    clEnqueueReleaseGLObjects(CLEnvironment::queue(),
                              2,
                              _depth_images,
                              0,
                              NULL,
                              NULL);
}

void SSpaceFluidRenderer::_render_normals_stage(const Camera& camera) {
    auto& gl = OpenGLFunctions::getFunctions();
    _normals_stage_shader->bind();

    // Configure shader uniforms
    auto perspective = camera.perspective();
    _normals_stage_shader->setUniformValue("inv_projection", camera.projection().inverted());

    // Configure the sampler
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, _depth_texture);

    // Bind depth fbo
    gl.glBindFramebuffer(GL_FRAMEBUFFER, _normals_fbo);
    gl.glClear(GL_COLOR_BUFFER_BIT);

    // Bind VAO and render the quad that fills the viewport
    gl.glBindVertexArray(_normals_stage_vao);

    // Render the quad
    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_indices_vbo);
    gl.glDrawElements(GL_TRIANGLES,
                      6, // The number of elements of the indices array
                      GL_UNSIGNED_SHORT,
                      0);

    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind fbo
}

void SSpaceFluidRenderer::_render_final_stage(const QMatrix4x4& mv_matrix,
                                              const Camera& camera, 
                                              GLuint dest_fbo, 
                                              GLuint bkg_texture, 
                                              GLuint cube_map_texture) {
    auto& gl = OpenGLFunctions::getFunctions();

    _final_stage_shader->bind();
    auto perspective = camera.perspective();
    _final_stage_shader->setUniformValue("near_plane", perspective.nearPlane);
    _final_stage_shader->setUniformValue("far_plane", perspective.farPlane);
    _final_stage_shader->setUniformValue("mv_matrix", mv_matrix);
    _final_stage_shader->setUniformValue("pr_matrix", camera.projection());
    _final_stage_shader->setUniformValue("inv_projection", camera.projection().inverted());

    // Bind default framebuffer (window)
    gl.glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);

    // Bind VAO and render the quad that fills the viewport
    gl.glBindVertexArray(_final_stage_vao);

    // Enable GL_TEXTURE_2D for using the previous stages textures
    gl.glActiveTexture(GL_TEXTURE0);
    gl.glBindTexture(GL_TEXTURE_2D, _depth_texture);

    gl.glActiveTexture(GL_TEXTURE1);
    gl.glBindTexture(GL_TEXTURE_2D, _normals_texture);

    gl.glActiveTexture(GL_TEXTURE2);
    gl.glBindTexture(GL_TEXTURE_2D, bkg_texture);

    gl.glActiveTexture(GL_TEXTURE3);
    gl.glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_texture);
    _final_stage_shader->setUniformValue("cube_map_texture", 3);

    gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quad_indices_vbo);
    gl.glDrawElements(GL_TRIANGLES,
                      6, // The number of elements of the indices array
                      GL_UNSIGNED_SHORT,
                      0);

    gl.glBindTexture(GL_TEXTURE_2D, 0);
}

void SSpaceFluidRenderer::reset(int particle_count) {
    _particle_count = particle_count;
}

SSpaceFluidRenderer::~SSpaceFluidRenderer() {  
    _release_final_stage();
    _release_normals_stage();
    _release_blur_stage();
    _release_depth_stage();
    _release_viewport_quad();
}

void SSpaceFluidRenderer::_release_depth_stage() {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glDeleteRenderbuffers(1, &_depth_fbo_depth_buffer);
    gl.glDeleteFramebuffers(1, &_depth_fbo);
    gl.glDeleteTextures(1, &_depth_texture);
    gl.glDeleteVertexArrays(1, &_depth_stage_vao);
}

void SSpaceFluidRenderer::_release_blur_stage() {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glDeleteTextures(1, &_blurred_depth_texture);
}

void SSpaceFluidRenderer::_release_normals_stage() {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glDeleteFramebuffers(1, &_normals_fbo);
    gl.glDeleteTextures(1, &_normals_texture);
    gl.glDeleteVertexArrays(1, &_normals_stage_vao);
}

void SSpaceFluidRenderer::_release_viewport_quad() {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glDeleteBuffers(1, &_quad_data_vbo);
    gl.glDeleteBuffers(1, &_quad_indices_vbo);
}

void SSpaceFluidRenderer::_release_final_stage() {
    auto& gl = OpenGLFunctions::getFunctions();

    gl.glDeleteVertexArrays(1, &_final_stage_vao);
}
