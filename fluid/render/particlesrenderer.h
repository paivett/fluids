#ifndef _PARTICLES_RENDERER_H_
#define _PARTICLES_RENDERER_H_

#include "fluidrenderer.h"

// Renders the fluid using the screen space fluid rendering technique
class ParticlesRenderer : public FluidRenderer {

    public:

        ParticlesRenderer(int viewport_width,
                          int viewport_height,
                          int particle_count,
                          GLuint vbo_particles);

        ~ParticlesRenderer();

        /**
         * @brief Renders the fluid in the scene
         * 
         * @param camera The current camera transformation
         * @param dir_lights The list of directional lights in the scene
         * @param mv_matrix The current model view transformation
         * @param dest_fbo The framebuffer objecto to use for rendering
         * @param background_texture This texture is the current rendered scene
         * @param cube_map_texture The cubemap texture used
         */
        void render(const Camera& camera,
                    const std::vector<DirectionalLight>& dir_lights,
                    const QMatrix4x4& mv_matrix,
                    GLuint dest_fbo,
                    GLuint background_texture,
                    GLuint bkg_depth_texture,
                    GLuint cube_map_texture);

        /**
         * @brief Resets the renderer
         * 
         * @param particle_count Number of fluid particles
         */
        void reset(int particle_count);

    private:
        // The vbo that holds the particles positions
        GLuint _vbo_particles;

        GLuint _vao;

        // The number of particles being rendered
        int _particle_count;

        // The original size of the viewport.
        int _viewport_w, _viewport_h;

        // Shader programs
        std::unique_ptr<QOpenGLShaderProgram> _shader;
};

#endif // _PARTICLES_RENDERER_H_