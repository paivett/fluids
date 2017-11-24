#ifndef PERSPECTIVE_H
#define PERSPECTIVE_H

struct Perspective
{
    float aspect;
    float fov;
    float nearPlane;
    float farPlane;

    // Viewport size
    int width, height;
};

#endif
