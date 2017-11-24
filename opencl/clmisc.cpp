#include "clmisc.h"

cl_ulong execution_time(cl_event& event) {
    cl_ulong time_start, time_end;
    cl_ulong total_time;

    clGetEventProfilingInfo(event, 
                            CL_PROFILING_COMMAND_START, 
                            sizeof(time_start), 
                            &time_start, 
                            NULL);

    clGetEventProfilingInfo(event, 
                            CL_PROFILING_COMMAND_END, 
                            sizeof(time_end),
                            &time_end, 
                            NULL);

    total_time = time_end - time_start;

    return total_time;
}
