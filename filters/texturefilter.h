#ifndef _TEXTURE_FILTER_H_
#define _TEXTURE_FILTER_H_

#include <opencl/clenvironment.h>

class TextureFilter {
    public:
        // Applies the filter to the origin image, and puts the result in the
        // filtered image
        virtual void filter(cl_mem input, cl_mem output) = 0;
};

#endif // _TEXTURE_FILTER_H_
