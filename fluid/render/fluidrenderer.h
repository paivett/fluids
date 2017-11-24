#ifndef _FLUID_RENDERER_H_
#define _FLUID_RENDERER_H_

#include "opengl/glutils.h"
#include "scene/camera.h"
#include "scene/light.h"
#include "settings/settings.h"
#include <QMatrix>

class FluidRenderer {

    public:

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
        virtual void render(const Camera& camera,
                            const std::vector<DirectionalLight>& dir_lights,
                            const QMatrix4x4& mv_matrix,
                            GLuint fbo,
                            GLuint background_texture,
                            GLuint bkg_depth_texture,
                            GLuint cube_map_texture) = 0;

        /**
         * @brief Resets the renderer
         * 
         * @param particle_count Number of fluid particles
         */
        virtual void reset(int particle_count) = 0;

        /**
         * Virtual destructor
         */
        virtual ~FluidRenderer() {};

};

#endif // _FLUID_RENDERER_H_