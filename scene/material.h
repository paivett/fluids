/**
 * Defines material structure and some utils
 */

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include <QColor>
#include <QVector3D>

struct Material {
    QColor ambient_color;
    QColor diffuse_color;
    QColor specular_color;
    float shininess;
};


/**
 * @brief Returns a random material from a pre existing list
 * @return A random material
 */
Material get_random_material();

#endif // __MATERIAL_H__