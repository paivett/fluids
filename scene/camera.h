#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <QMatrix4x4>
#include <QVector3D>

#include "perspective.h"

class Camera {
    public:
        Camera();

        // Sets the new perspective of the camera
        void set_perspective(const Perspective& p);

        // Returns the current perspective of the camera
        Perspective perspective() const;

        void look_at(const QVector3D& eye, const QVector3D& center, const QVector3D& up);

        // Returns projection matrix of the camera
        QMatrix4x4 projection() const;

        // Returns the transformation matrix of the camera
        QMatrix4x4 transformation() const;

        QMatrix4x4 rotation() const;

        // Adds pitch rotation to the camera. Values are in degrees
        void add_pitch(float pitch);

        // Adds roll rotation to the camera. Values are in degrees
        // and camera roll is restricted to [0, 90] degrees
        void add_roll(float roll);

        // Zooms in or out the camera. Zoom is restricted to the
        // perspective near and far plane
        void add_zoom(float zoom);

    private:
        QMatrix4x4 _projection;
        QMatrix4x4 _transformation;

        Perspective _perspective;

        float _pitch;
        float _roll;
        float _zoom;
};

#endif // _CAMERA_H_
