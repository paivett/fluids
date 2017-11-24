#ifndef _CL_PROGRAM_H_
#define _CL_PROGRAM_H_

#include <string>
#include <unordered_map>
#include <memory>
#include "clkernel.h"
#include "clenvironment.h"

/**
 * @class CLProgram
 * @brief Represents an OpenCL program
 * 
 * @details This class manages the creation of OpenCL programs.
 */
class CLProgram {
    
    public:
        /**
         * @brief Constructs the CLProgram given the program
         * 
         * @param p Instance of the OpenCL program
         */
        CLProgram(cl_program p);

        /**
         * @brief Unloads all resources
         */
        ~CLProgram();

        /**
         * @brief Returns a kernel instance
         * 
         * @param kernel_name The name of the kernel to retrieve from the program
         * @return The kernel
         */
        cl_kernel& get_native_kernel(const std::string& kernel_name);

        /**
         * @brief Returns a kernel instance
         * 
         * @param kernel_name The name of the kernel to retrieve from the program
         * @return The kernel
         */
         std::shared_ptr<CLKernel> get_kernel(const std::string& kernel_name);

    private:
        // Opencl program itself
        cl_program _program;
        
        // Dictionary of kernels within the program.
        // A new entry is created each time a kernel
        // is requested by get_kernel method and the
        // kernel exists in the program
        std::unordered_map<std::string, cl_kernel> _native_kernels;
        std::unordered_map<std::string, std::shared_ptr<CLKernel> > _kernels;
};

#endif // _CL_PROGRAM_H_
