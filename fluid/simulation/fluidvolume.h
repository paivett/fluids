#ifndef _FLUID_VOLUME_H_
#define _FLUID_VOLUME_H_

#include <vector>
#include <CL/cl.h>

class FluidVolume {

    public:
        virtual std::vector<cl_float4> particles(float particle_radius) = 0;

};

#endif // _FLUID_VOLUME_H_