#ifndef _CL_MISC_H_
#define _CL_MISC_H_

#include "external/type/type.h"
#include <CL/cl.h>
#include <string>
#include <cstring>

template<typename T>
std::string cl_type_to_str() {
    std::string type = demangle(typeid(T).name());
    // If type starts with cl_ then ignore prefix
    if (strncmp(type.c_str(), "cl_", 3) == 0) {
        type = type.substr(3);
    }

    return type;
}


cl_ulong execution_time(cl_event& event);


#endif // _CL_MISC_H_
