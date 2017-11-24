#ifndef _SCENE_OBJECT_H_
#define _SCENE_OBJECT_H_

#include <string>
#include <QOpenGLShaderProgram>
#include "camera.h"
#include "LinearMath/btTransform.h"
#include "light.h"
#include "material.h"

/**
 * @class SceneObject
 * @brief Represents an object inside the 3d scene
 */
class SceneObject {
    public:
        /**
         * @brief Default constructor
         * @details Creates a new scene object at the (0,0,0) world position,
         * no scaling, no rotation.
         */
        SceneObject();

        /**
         * @brief Initializes a SceneObject at a certain position, with no 
         * scaling or rotation.
         */
        SceneObject(float x, float y, float z);

        /**
         * @brief Creates a new scene object at the given position
         * 
         * @param pos Starting position
         */
        SceneObject(const btVector3& pos);

        /**
         * @brief Creates a new scene object at the given position, rotated by 
         * the given quaternion
         * 
         * @param pos Starting position
         * @param q Starting rotation
         */
        SceneObject(const btVector3& pos, const btQuaternion& q);

        /**
         * @brief Renders the object in the scene
         * 
         * @param cam Scene camera
         * @param dir_lights The list of directional lights being used
         * @param fbo The current FBO to render
         * @param texture The texture being used to render the color buffer
         * @param cube_map_texture The cube map texture being used
         */
        virtual void render(const Camera& cam,
                            const std::vector<DirectionalLight>& dir_lights,
                            GLuint fbo,
                            GLuint bkg_texture,
                            GLuint bkg_depth_texture,
                            GLuint cube_map_texture) = 0;

        /* Virtual destructor */
        virtual ~SceneObject();

        /**
         * @brief Returns the position of the object
         * @return The object position
         */
        virtual btVector3 position() const;

        /**
         * @brief Sets the position of the object
         * 
         * @param p Object position
         */
        virtual void set_position(const btVector3& p);

        /**
         * @brief Sets the position of the object
         * 
         * @param x X position
         * @param y Y position
         * @param z Z position
         */
        virtual void set_position(float x, float y, float z);

        /**
         * @brief Returns the rotation of the object
         * @return The object rotation
         */
        virtual btQuaternion rotation() const;

        /**
         * @brief Sets the rotation of the object
         * 
         * @param q Object rotation
         */
        virtual void set_rotation(const btQuaternion& q);

        /**
         * @brief Sets the material for this 
         * @details [long description]
         * 
         * @param mat [description]
         */
        void set_material(const Material& mat);

        /**
         * @brief Returns the material of this object
         * @return The material
         */
        Material material() const;

        /**
         * @brief Returns the object transform
         *  
         * @return The transform
         */
        virtual btTransform transform() const;

    protected:
        // Transformation matrix (rotation and translation)
        btTransform _transform;
        // Scale
        btVector3 _scale;
        // The material of this scene object. The material is 
        // thought to be used by the Phong shading model, but 
        // it can be reused partially to use other shading model 
        // like the fluid screen space curvature flow
        Material _material;
};

#endif // _SCENE_OBJECT_H_
