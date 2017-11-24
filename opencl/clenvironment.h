/** 
 *  @file clenvironment.h
 *  @brief Contains the declaration of the CLEnvironment class.
 *
 *  This contains the declaration of the class CLEnvironment, which facilitates
 *  access to a system device (the first GPU detected), and a unique command 
 *  queue.
 *
 *  @author Santiago Daniel Pivetta
 */

#ifndef _CL_ENVIRONMENT_H_
#define _CL_ENVIRONMENT_H_

#include <CL/cl.h>
#include <vector>
#include "clerror.h"

/**
 * @class CLEnvironment
 * @brief Provides easy access to device
 * @details Facilitates access to the first GPU detected. Provices a unique 
 *          command queue.
 */
class CLEnvironment {
    public:
        /**
         * @brief Initializes OpenCL state
         * @details Initializes the OpenCL state, with OpenGL interop enabled.
         * @note The init must be called AFTER the GL context
         *       has been initialized, otherwise the CL context 
         *       will not have GL-CL interoperability
         */
        static void init();

        /* Returns the first device detected of the first platform */
        static cl_device_id device();

        static cl_context& context();

        static cl_command_queue& queue();

    private:
        CLEnvironment();
        CLEnvironment(const CLEnvironment& e);
        CLEnvironment& operator=(const CLEnvironment& e);

        static std::vector<cl_platform_id> _platforms;
        static std::vector<cl_device_id> _devices;

        static cl_context _context;
        static cl_command_queue _queue;
};

#endif // _CL_ENVIROMENT_H_
