#ifndef _BOX_VOLUME_H_
#define _BOX_VOLUME_H_

#include "fluidvolume.h"
#include <LinearMath/btVector3.h>

class BoxVolume : public FluidVolume {

    public:
        BoxVolume(const btVector3& size, const btVector3& center);

        std::vector<cl_float4> particles(float particle_radius);

    private:
        btVector3 _size, _center;

};

#endif // _BOX_VOLUME_H_