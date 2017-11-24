#ifndef _CL_SHUFFLE_H_ 
#define _CL_SHUFFLE_H_

#include <typeinfo>
#include "opencl/clenvironment.h"
#include "opencl/clmisc.h"
#include <cmath>
#include <memory>


template<typename element_type>
int clshuffle(cl_mem buffer,
              cl_mem mask,
              cl_mem dest_buffer,
              int size,
              cl_event* event=nullptr) {

    static std::unique_ptr<CLProgram> program;
    static std::shared_ptr<CLKernel> kernel;
    
    if (!kernel) {
        CLCompiler compiler;
        std::string src = " \
            kernel void shuffle_buffer( \
            global <TYPE>* data, \
            global int* mask, \
            global <TYPE>* dest_data, \
            const int total_elements) { \
                int global_id = get_global_id(0); \
 \
                if (global_id < total_elements) { \
                    int e_pos = mask[global_id]; \
                    dest_data[global_id] = data[e_pos]; \
                } \
            }";
        // Replace all ocurrences of <TYPE> with the correct type
        std::string::size_type n = 0;
        std::string type = cl_type_to_str<element_type>(); 
        while ((n = src.find("<TYPE>", n)) != std::string::npos) {
            src.replace(n, 6, type);
            n += type.size();
        }
        compiler.add_source(src);
        compiler.add_build_option("-cl-std=CL1.2");
        compiler.add_build_option("-cl-fast-relaxed-math");
        program = compiler.build();

        // Initialize reorder kernel
        kernel = program->get_kernel("shuffle_buffer");
    }

    kernel->set_arg(0, &buffer);
    kernel->set_arg(1, &mask);
    kernel->set_arg(2, &dest_buffer);
    kernel->set_arg(3, &size);

    return kernel->run(size);
}

#endif