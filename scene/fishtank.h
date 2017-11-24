#ifndef _FISH_TANK_H_
#define _FISH_TANK_H_

#include "opengl/openglfunctions.h"
#include "mesh/wireboxmesh.h"
#include "rigidbody.h"
#include <memory>
#include <QColor>

class FishTank : public RigidBody {
    public:
        //Builds a box of width, height and depth size, centered at (0,0,0)
        FishTank(float width, float height, float depth, float mass);

        //Destructor releases all resources of the box
        ~FishTank();

        /**
         * @brief Renders the tank in the scene
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

        //Configure grids color
        void set_color(float r, float g, float b, float alpha = 1.0);
        void set_color(const QColor& color);

        QColor color() const;

        float width() const;

        float height() const;

        float depth() const;

        /**
         * @brief Returns a particle sampling of the surface of the fish tank
         * 
         * @param particle_spacing The spacing between particles.
         * @return A vector of positions in space, each for every particle.
         */
        std::vector<btVector3> surface_sampling(float particle_spacing) const;

    private:
        float _width, _height, _depth;
        QColor _color;

        std::unique_ptr<WireBoxMesh> _box_mesh;

        // Shader program
        std::unique_ptr<QOpenGLShaderProgram> _shader;

        GLuint _vao;
        GLuint _vbo_ids[2];
        QMatrix4x4 _qt_transformation;

        /**
         * @brief Initializes physics of the cube
         */
        void _init_physics();
};

#endif // _FISH_TANK_H_
