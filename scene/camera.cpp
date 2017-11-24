#include "camera.h"
#include <algorithm>

Camera::Camera(): _pitch(0), _roll(45), _zoom(4) {
    _projection.setToIdentity();
    _transformation.setToIdentity();
}

QMatrix4x4 Camera::projection() const {
    return _projection;
}

QMatrix4x4 Camera::transformation() const {
    QMatrix4x4 t;
    t.setToIdentity();
    t.translate(0, 0, -_zoom);
    t.rotate(_roll, 1, 0, 0);
    t.rotate(_pitch, 0, 1, 0);

    return t;
}

QMatrix4x4 Camera::rotation() const {
    QMatrix4x4 t;
    t.setToIdentity();
    t.rotate(_roll, 1, 0, 0);
    t.rotate(_pitch, 0, 1, 0);

    return t;
}

void Camera::set_perspective(const Perspective& p) {
    _perspective = p;
    _projection.setToIdentity();
    _projection.perspective(p.fov, p.aspect, p.nearPlane, p.farPlane);
}

Perspective Camera::perspective() const {
    return _perspective;
}

void Camera::look_at(const QVector3D& eye, const QVector3D& center, const QVector3D& up) {
    _transformation.setToIdentity();
    _transformation.lookAt(eye, center, up);
}

void Camera::add_pitch(float p) {
    _pitch += p;
    if (_pitch < 0)
    {
        _pitch += 360;
    }
    else if (_pitch > 360)
    {
        _pitch -= 360;
    }
}

void Camera::add_roll(float r) {
    _roll = std::min(std::max(_roll + r, 0.0f), 90.0f);
}

void Camera::add_zoom(float z) {
    _zoom = std::min(std::max(_zoom + z, _perspective.nearPlane), _perspective.farPlane);
}
