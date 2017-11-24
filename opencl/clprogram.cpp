#include "clprogram.h"
#include "clenvironment.h"
#include <stdexcept>
#include <fstream>

using namespace std;

CLProgram::CLProgram(cl_program p) : _program(p) {

}

cl_kernel& CLProgram::get_native_kernel(const std::string& kernel_name) {
    cl_int err;
    auto i = _native_kernels.find(kernel_name);
    if (i != _native_kernels.end()) {
        return i->second;
    }
    else {
        cl_kernel k = clCreateKernel(_program, kernel_name.c_str(), &err);
        CLError::check(err);
        _native_kernels[kernel_name] = k;
        return _native_kernels[kernel_name];
    }
}

shared_ptr<CLKernel> CLProgram::get_kernel(const std::string& kernel_name) {
    auto i = _kernels.find(kernel_name);
    if (i != _kernels.end()) {
        return i->second;
    }
    else {
        auto native_kernel = get_native_kernel(kernel_name);
        auto p = make_shared<CLKernel>(native_kernel);
        _kernels[kernel_name] = p;
        return p;
    }
}

CLProgram::~CLProgram() {
    for (auto& p : _native_kernels) {
        clReleaseKernel(p.second);
    }

    clReleaseProgram(_program);
}