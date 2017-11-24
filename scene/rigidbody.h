#ifndef _RIGID_BODY_H_
#define _RIGID_BODY_H_

#include <memory>
#include "sceneobject.h"
#include "mesh/mesh.h"
#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"


class Scene;

/**
 * @class RigidBody
 * @brief Represents a rigid body.
 * @details A rigid body within the scene, that can interact with other rigid
 * bodies, according to the laws of mechanics, and also may interact with
 * the fluid simulation through the particles that sample the surface.
 * 
 */
class RigidBody : public SceneObject {
    public:
        friend class Scene;

        /**
         * @brief Initializes rigid body with a mass at location (0,0,0)
         * 
         * @param mass Body mass
         */ 
        RigidBody(float mass);

        /**
         * @brief Initializes a new rigid body
         * 
         * @param mass Body mass
         * @param x X position
         * @param y Y position
         * @param z Z position
         */
        RigidBody(float mass, float x, float y, float z);

        /**
         * @brief Initializes a new rigid body
         * 
         * @param mass Body mass
         * @param pos The initial position
         * @param q The initial rotation
         */
        RigidBody(float mass, const btVector3& pos, const btQuaternion& q);

        /**
         * @brief Returns the mass of the rigid body
         * @return Rigid body mass
         */
        float mass() const;

        /**
         * @brief Returns the rotation of the object
         * @return The object rotation
         */
        btQuaternion rotation() const;

        /**
         * @brief Sets the rotation of the object
         * 
         * @param q Object rotation
         */
        void set_rotation(const btQuaternion& q);

        /**
         * @brief Set rigid body position
         * 
         * @param x X position
         * @param y Y position
         * @param z Z position
         */
        void set_position(float x, float y, float z);

        /**
         * @brief Set rigid body position
         *
         * @param pos The initial position
         */
        void set_position(const btVector3& pos);

        /**
         * @brief Returns the center of mass position
         * @return Position
         */
        btVector3 position() const;

        /**
         * @brief Returns the center of mass current transformation
         * @return The center of mass transformation
         */
        btTransform transform() const;
      
        /**
         * @brief Returns a particle sampling of the surface of the rigid body
         * 
         * @param particle_spacing The spacing between particles.
         * @return A vector of positions in space, each for every particle.
         */
        virtual std::vector<btVector3> surface_sampling(float particle_spacing) const = 0;

        /**
         * @brief Returns the angular velocity of the center of mass
         * @return The angular vel
         */
        btVector3 angular_vel() const;

        /**
         * @brief Returns the linear velocity of the center of mass
         * @return The velocity
         */
        btVector3 linear_vel() const;

        /**
         * @brief Applies a given force to the center of mass
         * 
         * @param f Force to apply
         */
        void apply_force_n_torque(const btVector3& f, const btVector3& t);

    protected:    
        // Rigid body mass
        float _mass;

        btRigidBody* _bt_rigid_body;
        btCollisionShape* _bt_collision_shape;
        btVector3 _local_inertia;
        btDefaultMotionState* _motion_state;

        /**
         * @brief Resets internal motion state
         */
        void _reset_motion_state();
};

#endif // _RIGID_BODY_H_