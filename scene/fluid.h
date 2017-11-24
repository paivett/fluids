#ifndef _FLUID_H_
#define _FLUID_H_

#include "fluid/simulation/fluidsimulation.h"
#include "fluid/simulation/fluidsimulationfactory.h"
#include "opengl/openglfunctions.h"
#include "filters/bilateralfilter.h"
#include "sceneobject.h"
#include "rigidbody.h"
#include "fluid/render/fluidrenderer.h"
#include <memory>
#include <QColor>


class Fluid : public SceneObject {
    public:
        // Initializes a fluid object
        Fluid(int viewport_width,
              int viewport_height,
              const PhysicsSettings& fluid_settings,
              const SimulationSettings& sim_settings,
              const GraphicsSettings& g_settings);

        /**
         * @brief Releases all resources of the fluid
         */
        ~Fluid();

        /**
         * @brief Renders the fluid in the scene
         * 
         * @param cam Scene camera
         * @param dir_lights The list of directional lights being used
         * @param fbo The current FBO to render
         * @param texture The texture being used to render the color buffer
         * @param cube_map_texture The cube map texture being used
         */
        void render(const Camera& camera,
                    const std::vector<DirectionalLight>& dir_lights,
                    GLuint dest_fbo,
                    GLuint background_texture,
                    GLuint bkg_depth_texture,
                    GLuint cube_map_texture);

        FluidSimulation& simulation();

        /**
         * Simulates the fluid a dt step (dt has been configured when setting up 
         * the fluid)
         */
        void simulate();

        /**
         * @brief Sets the color of the cube, used during rendering. All values
         * must go from 0 to 1
         * 
         * @param r Red component
         * @param g Green component
         * @param b Blue component
         * @param alpha Alpha component
         */
        void set_color(float r, float g, float b, float alpha = 1.0);
        
        /**
         * @brief Sets the color of the cube, used during rendering.
         * 
         * @param color QColor instance
         */
        void set_color(const QColor& color);

        /**
         * @brief Returns the current color being used for rendering
         * @return A QColor instance
         */
        QColor color() const;

        /**
         * @brief Resets the internal state of the fluid
         * @details Use this method to reset all the properties of the fluid, 
         * the parameters that affect the fluid solver, and also the renderer
         * 
         * @param s_settings Settings related to the simulation method
         * @param p_settings Settings related to the physics of the fluid
         * @param g_settings Settings related to the rendering of the fluid
         */
        void reset(const SimulationSettings& s_settings,
                   const PhysicsSettings& p_settings,
                   const GraphicsSettings& g_settings);

        /**
         * @brief Adds a new rigid body to the fluid as a boundary. Depending
         *        on the mass, it will be treated as static or dynamic.
         * 
         * @param boundary The boundary
         */
        void add_boundary(const std::shared_ptr<RigidBody> boundary,
                          const std::string& id,
                          bool can_move=false);

        /**
         * @brief Add a new volume to the fluid
         * 
         * @param volume Fluid volume
         */
        void add_volume(std::shared_ptr<FluidVolume> volume);

        /**
         * @brief Returns the number of fluid particles.
         */
        int particle_count() const;

        /**
         * @brief Returns the number of particles used to model the boundary.
         */
        int boundary_particle_count() const;

        void set_rect_limits(float width, float height, float depth);

    private:
        int _viewport_w, _viewport_h;

        QColor _color;

        // Fluid simulation, this is the one that calculates
        // particles positions
        std::unique_ptr<FluidSimulation> _simulation;

        // OpenGL particles buffers
        GLuint _vbo_fluid_particles; // Particle's centers
        GLuint _vbo_boundary_particles; // Particles mass densities
        QMatrix4x4 _qt_transformation;

        std::unique_ptr<FluidRenderer> _renderer;

        void _init_renderer(const PhysicsSettings& fluid_settings,
                            const SimulationSettings& sim_settings,
                            const GraphicsSettings& g_settings);
};

#endif // _FLUID_H_
