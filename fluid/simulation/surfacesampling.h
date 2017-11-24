#ifndef __SURFACE_SAMPLING_H__
#define __SURFACE_SAMPLING_H__

#include <vector>
#include <LinearMath/btVector3.h>

struct SurfaceSampling {
    std::vector<btVector3> particles;
    std::vector<btVector3> normals;
};

#endif // __SURFACE_SAMPLING_H__