#ifndef _SSPACE_FLUID_RENDERER_H_
#define _SSPACE_FLUID_RENDERER_H_

#include "fluidrenderer.h"
#include "filters/bilateralfilter.h"

// Renders the fluid using the screen space fluid rendering technique
class SSpaceFluidRenderer : public FluidRenderer {

    public:

        SSpaceFluidRenderer(int viewport_width,
                            int viewport_height,
                            int filter_iterations,
                            int particle_count,
                            GLuint vbo_particles);

        ~SSpaceFluidRenderer();

        /**
         * @brief Renders the fluid in the scene
         * 
         * @param camera The current camera transformation
         * @param dir_lights The list of directional lights in the scene
         * @param mv_matrix The current model view transformation
         * @param fbo The framebuffer objecto to use for rendering
         * @param background_texture This texture is the current rendered scene
         * @param cube_map_texture The cubemap texture used
         */
        void render(const Camera& camera,
                    const std::vector<DirectionalLight>& dir_lights,
                    const QMatrix4x4& mv_matrix,
                    GLuint fbo,
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

        // The number of particles being rendered
        int _particle_count;

        // The filter function to apply to the depth buffer
        BilateralFilter _depth_filter;
        
        // This two cl_mem buffers are used for the opengl-opencl interop
        // when filtering the depth buffer
        cl_mem _depth_images[2];
        
        // The number of iterations the filter will be applied. Consider that
        // for each iterations, the filter is applied twice
        int _filter_iterations;

        // The original size of the viewport. For performace, the viewport may
        // be reduced during the filtering stage.
        int _viewport_w, _viewport_h;

        // Shader programs
        std::unique_ptr<QOpenGLShaderProgram> _depth_stage_shader;
        std::unique_ptr<QOpenGLShaderProgram> _blur_stage_shader;
        std::unique_ptr<QOpenGLShaderProgram> _normals_stage_shader;
        std::unique_ptr<QOpenGLShaderProgram> _final_stage_shader;

        // This VBO holds the quad data (vertices + texture coordinates)
        // used by some stages to render to a quad that simulates the 
        // viewport
        GLuint _quad_data_vbo;
        GLuint _quad_indices_vbo;

        // VAO's for the different stages
        GLuint _depth_stage_vao;
        GLuint _blur_stage_vao;
        GLuint _normals_stage_vao;
        GLuint _final_stage_vao;

        // Framebuffers
        GLuint _depth_fbo, _depth_fbo_depth_buffer;
        GLuint _blur_fbo;
        GLuint _normals_fbo;

        // Textures for the fbo's
        GLuint _depth_texture;
        GLuint _blurred_depth_texture;
        GLuint _normals_texture;

        // Initialization functions
        void _init_viewport_quad();
        void _init_depth_stage(int width, int height);
        void _init_blur_stage(int width, int height);
        void _init_normals_stage(int width, int height);
        void _init_final_stage();

        // Release functions
        void _release_viewport_quad();
        void _release_depth_stage();
        void _release_blur_stage();
        void _release_normals_stage();
        void _release_final_stage();

        // Rendering stages
        void _render_depth_stage(const QMatrix4x4& mv_matrix, const Camera& camera, GLuint bkg_depth_texture);
        void _render_blur_stage(const QMatrix4x4& mv_matrix, const Camera& camera);
        void _render_normals_stage(const Camera& camera);
        void _render_final_stage(const QMatrix4x4& mv_matrix, const Camera& camera, GLuint bkg_fbo, GLuint bkg_texture, GLuint cube_map_texture);

};

#endif // _SSPACE_FLUID_RENDERER_H_