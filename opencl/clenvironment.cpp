#include "clenvironment.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <CL/cl_gl.h>

using namespace std;

// Static memebers initialization
vector<cl_platform_id> CLEnvironment::_platforms;
vector<cl_device_id> CLEnvironment::_devices;
cl_context CLEnvironment::_context;
cl_command_queue CLEnvironment::_queue;

void CLEnvironment::init() {
    cl_int status;

    // Query platforms
    cl_uint num_platforms;
    clGetPlatformIDs(10, NULL, &num_platforms);
    _platforms.resize(num_platforms);
    clGetPlatformIDs(num_platforms, _platforms.data(), NULL);

    // List devices of the first platform available
    cl_uint num_devices;
    // clGetDeviceIDs(_platforms[0], CL_DEVICE_TYPE_GPU, 10, NULL, &num_devices);
    clGetDeviceIDs(_platforms[0], CL_DEVICE_TYPE_GPU, 10, NULL, &num_devices);
    _devices.resize(num_devices);
    // clGetDeviceIDs(_platforms[0], CL_DEVICE_TYPE_GPU, 10, _devices.data(), NULL);
    clGetDeviceIDs(_platforms[0], CL_DEVICE_TYPE_GPU, 10, _devices.data(), NULL);

    cl_context_properties properties[] = {
      CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
      CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
      CL_CONTEXT_PLATFORM, (cl_context_properties) _platforms[0],
      0};

    _context = clCreateContext(properties,
                               num_devices,
                               _devices.data(),
                               NULL,
                               NULL,
                               &status);
    CLError::check(status);

    _queue = clCreateCommandQueue(_context,
                                  _devices[0],
                                  CL_QUEUE_PROFILING_ENABLE,
                                  &status);
    CLError::check(status);
}

cl_context& CLEnvironment::context() {
    return CLEnvironment::_context;
}

cl_device_id CLEnvironment::device() {
    return CLEnvironment::_devices[0];
}

cl_command_queue& CLEnvironment::queue() {
    return CLEnvironment::_queue;
}
