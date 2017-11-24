#ifndef _CL_KERNEL_H_
#define _CL_KERNEL_H_

#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>
#include "clenvironment.h"
#include "clmisc.h"
#include "clrange.h"

#include <iostream>

#define _KERNEL_OPT_ROUNDS  200

/**
 * @class CLKernel
 * @brief Represents an OpenCL kernel
 * 
 * @details This class represents OpenCL kernels, and encapsulates, not only 
 * the management of kernel stuff like arguments, but also provides the autotune 
 * capability
 */

  class CLKernel {
    
    public:
        /**
         * @brief Constructs the CLKernel
         * 
         * @param k Instance of the OpenCL kernel
         */
        CLKernel(cl_kernel k);

        /**
         * @brief Releases the kernel
         */
        ~CLKernel();

        /**
         * @brief Returns the name of the kernel
         */
        std::string name() const;

        template<typename T>
        cl_int set_arg(cl_uint index, const T* value) {
            return clSetKernelArg(_kernel, index, sizeof(T), value);
        }

        void set_local_buffer(cl_uint index, size_t type_size);

        cl_int run(const CLRange& work_item_count, 
                   const CLRange& local_size,
                   const CLRange& opt_step_size);

        cl_int run(size_t work_item_count, size_t local_size=0);

    private:      
        // Opencl kernel itself
        cl_kernel _kernel;

        struct _OptimizationData {
            CLRange local_size, global_size;
            std::unordered_map<cl_uint, cl_ulong> accum_time;
            std::unordered_map<cl_uint, CLRange> range;
            int rounds;

            _OptimizationData() : rounds(0) {}
        };
        
        std::unordered_map<size_t, _OptimizationData> _opt_data;

        std::vector<std::pair<cl_uint, size_t> > _local_buffers_info;

        std::string _kernel_name;

        std::string _prof_filename;
};

#endif // _CL_KERNEL_H_
