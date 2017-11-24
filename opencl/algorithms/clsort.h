#ifndef _CL_SORT_H_
#define _CL_SORT_H_

#include "opencl/clenvironment.h"
#include <clogs/clogs.h>
#include <typeindex>
#include "external/boost/compute.hpp"

clogs::Type __clType_2_clogsType(std::type_index t_idx);


template<typename key_type, typename value_type>
int clsort(cl_mem keys, cl_mem values, int size, cl_event* event=nullptr) {
    // Initialize clog sorter
    static clogs::Radixsort sorter(CLEnvironment::context(),
                                   CLEnvironment::device(),
                                   __clType_2_clogsType(typeid(key_type)),
                                   __clType_2_clogsType(typeid(value_type)));

    try {
        sorter.enqueue(CLEnvironment::queue(),
                       keys,
                       values,
                       size,
                       0,
                       0,
                       NULL,
                       event);
    }
    catch (const clogs::Error& e) {
        return e.err();
    }

    return CL_SUCCESS;
}

#endif // _CL_SORT_H_