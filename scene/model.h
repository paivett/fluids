#ifndef _MODEL_H_
#define _MODEL_H_

#include "opengl/openglfunctions.h"
#include "mesh/modelmesh.h"
#include "rigidbody.h"
#include <memory>
#include <QColor>

/**
 * @class Model
 * @brief A rigid body based on a custom model mesh
 */
class Model : public RigidBody {
    public:
        /**
         * @brief Builds a new scene object based on a custom mesh model
         * 
         * @param model_filename The filename of the OBJ Wavefront file
         * @param mass The mass of this new rigid body
         * @param pos The initial position in space
         * @param q The initial rotation
         */
        Model(const std::string& model_filename, 
              float mass,
              const btVector3& pos,
              const btQuaternion& q);

        /* Destructor */
        ~Model();

        /**
         * @brief Renders the model in the scene
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

        /**
         * @brief Returns a particle sampling of the surface of model
         * 
         * @param particle_spacing The spacing between particles.
         * @return A vector of positions in space, each for every particle.
         */
        std::vector<btVector3> surface_sampling(float particle_spacing) const;

    private:
        // The mesh of the cube
        std::unique_ptr<ModelMesh> _model_mesh;

        // Shader program
        std::unique_ptr<QOpenGLShaderProgram> _shader;

        GLuint _vao;
        GLuint _vbo_ids[3];
        QMatrix4x4 _qt_transformation;

        btTriangleMesh* _bt_mesh_interface;

        /**
         * @brief Initializes physics of the model
         */
        void _init_physics();
};

#endif // _MODEL_H_
