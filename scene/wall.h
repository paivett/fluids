#ifndef _WALL_H_
#define _WALL_H_

#include "opengl/openglfunctions.h"
#include "mesh/quadmesh.h"
#include "rigidbody.h"
#include <memory>

/**
 * @class Wall
 * @brief A simple flat wall.
 */
class Wall : public RigidBody {
    public:
        /**
         * @brief Wall constructor
         * @details Creates a new wall with a given width, height and mass, at 
         * a defined position, and rotation.
         * 
         * @param width The width of the wall
         * @param height The height of the wall
         * @param mass Cube mass
         * @param pos Initial position
         * @param q Initial rotation
         */
        Wall(float width, 
             float height,
             float mass,
             const btVector3& pos,
             const btQuaternion& q);

        /* Destructor */
        ~Wall();

        /**
         * @brief Renders the wall in the scene
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

        /* Returns the width of the wall */
        float width() const;

        /* Returns the height of the wall */
        float height() const;

        /**
         * @brief Returns a particle sampling of the surface of the wall
         * 
         * @param particle_spacing The spacing between particles.
         * @return A vector of positions in space, each for every particle.
         */
        std::vector<btVector3> surface_sampling(float particle_spacing) const;

    private:
        // The side of the cube
        float _width, _height;

        // The mesh of the cube
        std::unique_ptr<QuadMesh> _quad_mesh;

        // Shader program
        std::unique_ptr<QOpenGLShaderProgram> _shader;

        GLuint _vao;
        GLuint _vbo_ids[3];
        QMatrix4x4 _qt_transformation;

        /**
         * @brief Initializes physics of the cube
         */
        void _init_physics();
};

#endif // _WALL_H_
