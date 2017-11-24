#include "bilateralfilter.h"

BilateralFilter::BilateralFilter(int width,
                                 int height) :
_width(width),
_height(height) {
    CLCompiler compiler;
    compiler.add_file_source("kernels/bilateral.cl");
    compiler.add_build_option("-cl-std=CL1.2");
    compiler.add_build_option("-cl-fast-relaxed-math");
    compiler.add_include_path("kernels");
    _program = compiler.build();

    _kernel_filter = _program->get_native_kernel("bilateral");

    int ls = 8;
    _global_size[0] = _width + ls - (_width % ls);
    _global_size[1] = _height + ls - (_height % ls);

    _local_size[0] = ls;
    _local_size[1] = ls;

    clSetKernelArg(_kernel_filter, 2, sizeof(cl_int), &_width);
    clSetKernelArg(_kernel_filter, 3, sizeof(cl_int), &_height);
}

void BilateralFilter::filter(cl_mem input, cl_mem output) {
    clSetKernelArg(_kernel_filter, 0, sizeof(cl_mem), &input);
    clSetKernelArg(_kernel_filter, 1, sizeof(cl_mem), &output);

    clEnqueueNDRangeKernel(CLEnvironment::queue(),
                           _kernel_filter,
                           2,
                           NULL,
                           _global_size,
                           _local_size,
                           0,
                           NULL,
                           NULL);
}
