#ifndef _BILATERAL_FILTER_H_
#define _BILATERAL_FILTER_H_

#include "texturefilter.h"
#include <opencl/clenvironment.h>
#include <opencl/clcompiler.h>
#include <opengl/openglfunctions.h>


class BilateralFilter {
    public:
        // Create a new filter
        BilateralFilter(int width, int height);

        void filter(cl_mem input, cl_mem output);

    private:
        // Size of the texture to filter
        int _width, _height;

        std::unique_ptr<CLProgram> _program;
        cl_kernel _kernel_filter;
        size_t _global_size[2];
        size_t _local_size[2];
};

#endif // _BILATERAL_FILTER_H_
