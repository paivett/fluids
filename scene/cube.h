#ifndef _CUBE_H_
#define _CUBE_H_

#include "opengl/openglfunctions.h"
#include "mesh/boxmesh.h"
#include "rigidbody.h"
#include <memory>

/**
 * @class Cube
 * @brief A simple cube.
 */
class Cube : public RigidBody {
    public:
        /**
         * @brief Cube constructor
         * @details Creates a new cube with a given side length and mass, at a 
         * defined position
         * 
         * @param side The side length
         * @param mass Cube mass
         * @param pos Initial position
         * @param q Initial rotation
         */
        Cube(float side, 
             float mass,
             const btVector3& pos,
             const btQuaternion& q);

        /* Destructor */
        ~Cube();

        /**
         * @brief Renders the cube in the scene
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

        /* Returns the side of the cube */
        float side() const;

        /**
         * @brief Returns a particle sampling of the surface of the cube
         * 
         * @param particle_spacing The spacing between particles.
         * @return A vector of positions in space, each for every particle.
         */
        std::vector<btVector3> surface_sampling(float particle_spacing) const;

    private:
        // The side of the cube
        float _side;

        // The mesh of the cube
        std::unique_ptr<BoxMesh> _box_mesh;

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

#endif // _BOX_H_
