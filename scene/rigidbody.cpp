#include "rigidbody.h"
#include <iostream>

RigidBody::RigidBody(float mass) : 
_mass(mass),
_bt_rigid_body(nullptr),
_bt_collision_shape(nullptr) {

}

RigidBody::RigidBody(float mass, float x, float y, float z) : 
SceneObject(x,y,z),
_mass(mass),
_bt_rigid_body(nullptr),
_bt_collision_shape(nullptr) {

}

RigidBody::RigidBody(float mass, const btVector3& pos, const btQuaternion& q) : 
SceneObject(pos, q),
_mass(mass),
_bt_rigid_body(nullptr),
_bt_collision_shape(nullptr) {

}

float RigidBody::mass() const {
    return _mass;
}

void RigidBody::set_position(float x, float y, float z) {
    set_position(btVector3(x, y, z));
}

void RigidBody::set_position(const btVector3& pos) {
    auto t = _bt_rigid_body->getCenterOfMassTransform();
    t.setOrigin(pos);
    _bt_rigid_body->setCenterOfMassTransform(t);
}

btTransform RigidBody::transform() const {
    return _bt_rigid_body->getCenterOfMassTransform();
}

btVector3 RigidBody::position() const {
    return _bt_rigid_body->getCenterOfMassTransform().getOrigin();
}

void RigidBody::apply_force_n_torque(const btVector3& f, const btVector3& t) {
    _bt_rigid_body->setSleepingThresholds(0, 0);
    _bt_rigid_body->clearForces();
    _bt_rigid_body->applyCentralForce(f);
    _bt_rigid_body->applyTorque(t);
}

btVector3 RigidBody::angular_vel() const {
    return _bt_rigid_body->getAngularVelocity();
}

btVector3 RigidBody::linear_vel() const {
    return _bt_rigid_body->getLinearVelocity();
}

btQuaternion RigidBody::rotation() const {
    return _bt_rigid_body->getCenterOfMassTransform().getRotation();
}

void RigidBody::set_rotation(const btQuaternion& q) {
    _reset_motion_state();
    auto t = _bt_rigid_body->getCenterOfMassTransform();
    t.setRotation(q);
    _bt_rigid_body->setCenterOfMassTransform(t);
}

void RigidBody::_reset_motion_state() {
    _bt_rigid_body->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    _bt_rigid_body->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
    _bt_rigid_body->clearForces();
}
