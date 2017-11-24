#include "sceneobject.h"
#include "runtimeexception.h"

using namespace std;

SceneObject::SceneObject() : _transform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)) {

}

SceneObject::SceneObject(float x, float y, float z) : _transform(btQuaternion(0, 0, 0, 1), btVector3(x, y, z)) {

}


SceneObject::SceneObject(const btVector3& pos) : _transform(btQuaternion(0, 0, 0, 1), pos) {

}

SceneObject::SceneObject(const btVector3& pos, const btQuaternion& q) : _transform(q, pos) {

}

SceneObject::~SceneObject() {

}

btVector3 SceneObject::position() const {
    return _transform.getOrigin(); 
}

void SceneObject::set_position(float x, float y, float z) {
    _transform.setOrigin(btVector3(x, y, z));
}

void SceneObject::set_position(const btVector3& p) {
    _transform.setOrigin(p);
}

void SceneObject::set_material(const Material& m) {
    _material = m;
}

Material SceneObject::material() const {
    return _material;
}

btQuaternion SceneObject::rotation() const {
    return _transform.getRotation();
}

void SceneObject::set_rotation(const btQuaternion& q) {
    _transform.setRotation(q);
}

btTransform SceneObject::transform() const {
    return _transform;
}