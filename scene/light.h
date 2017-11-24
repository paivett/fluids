/**
 * Defines light structures and utils for Phong per pixel lighting model
 */

#ifndef __LIGHT_H__
#define __LIGHT_H__

#include <QColor>
#include <QVector3D>

struct DirectionalLight {
    QVector3D direction;
    QColor ambient_color;
    QColor diffuse_color;
    QColor specular_color;
    QColor attenuation;
};

#endif // __LIGHT_H__