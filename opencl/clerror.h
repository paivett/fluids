#ifndef _CL_ERROR_H_
#define _CL_ERROR_H_

#include <CL/cl.h>
#include <stdexcept>

class CLError : public std::runtime_error {

    public:
        CLError(cl_int error);

        // Returns a description of the error
        virtual const char* what() const noexcept (true);

        // Throws exception if error is not CL_SUCCESS
        static void check(cl_int error);

    private:
        // OpenCL error code
        cl_int _cl_error;

        // Message to be returned by what() method
        std::string _message;

        // Returns a string representation of the error code
        static const char* error_to_string(cl_int code);
};

#endif // _CL_ERROR_H_
