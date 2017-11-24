#ifndef _SPHERE_H_
#define _SPHERE_H_

#include "opengl/openglfunctions.h"
#include "mesh/icospheremesh.h"
#include "rigidbody.h"
#include <memory>
#include <QColor>

/**
 * @class Sphere
 * @brief A simple sphere.
 */
class Sphere : public RigidBody {
    public:
        /**
         * @brief Sphere constructor
         * @details Creates a new sphere with a given radius at a defined
         * defined position
         *
         * @param radius The radius of the sphere
         * @param mass Cube mass
         * @param x Initial x position
         * @param y Initial y position
         * @param z Initial z position
         */
        Sphere(float radius,
               float mass,
               const btVector3& pos);

        /* Destructor */
        ~Sphere();

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

        /* Returns the radius of the sphere */
        float radius() const;

        /**
         * @brief Returns a particle sampling of the surface of the sphere
         *
         * @param particle_spacing The spacing between particles.
         * @return A vector of positions in space, each for every particle.
         */
        std::vector<btVector3> surface_sampling(float particle_spacing) const;

    private:
        // The radius of the sphere
        float _radius;

        // The color to use for rendering
        QColor _color;

        // The mesh of the sphere
        std::unique_ptr<IcoSphereMesh> _sphere_mesh;

        // Shader program
        std::unique_ptr<QOpenGLShaderProgram> _shader;

        GLuint _vao;
        GLuint _vbo_ids[3];
        QMatrix4x4 _qt_transformation;

        /**
         * @brief Initializes physics of the sphere
         */
        void _init_physics();
};

#endif // _SPHERE_H_
