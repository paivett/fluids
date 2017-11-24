#ifndef _CL_REDUCE_H_ 
#define _CL_REDUCE_H_

#include "opencl/clenvironment.h"
#include "external/boost/compute.hpp"

template<typename T>
struct _TypeTranslator {
    typedef T type;

    static T convert(type& x) { return x; }
};

template<>
struct _TypeTranslator<cl_float4> {
    typedef boost::compute::float4_ type;

    static cl_float4 convert(type& x) {
        cl_float4 y;
        for (int i=0; i<4; ++i) {
            y.s[i] = x[i];
        }
        return y;
    }
};

template<typename element_type>
using max_reduce = boost::compute::max<element_type>;

template<typename element_type>
using min_reduce = boost::compute::min<element_type>;

template<typename element_type>
using sum_reduce = boost::compute::plus<element_type>;

template<typename element_type, typename binary_func>
int clreduce(cl_mem buffer, int size, element_type& result, binary_func f) {
    auto boost_ctx = boost::compute::context(CLEnvironment::context());
    auto boost_queue = boost::compute::command_queue(CLEnvironment::queue());
    auto boost_buffer = boost::compute::buffer(buffer);

    auto begin = boost::compute::make_buffer_iterator<element_type>(boost_buffer, 0);
    auto end = boost::compute::make_buffer_iterator<element_type>(boost_buffer, size);

    boost::compute::reduce(begin,
                           end, 
                           &result, 
                           f,
                           boost_queue);

    return CL_SUCCESS;
}

template<typename T>
int clmax(cl_mem buffer, 
          int size, 
          typename _TypeTranslator<T>::type& result) {
    return clreduce(buffer, 
                    size, 
                    result, 
                    max_reduce<typename _TypeTranslator<T>::type>());
}

template<typename T>
int clmin(cl_mem buffer, 
          int size, 
          typename _TypeTranslator<T>::type& result) {
    return clreduce(buffer, 
                    size, 
                    result, 
                    min_reduce<typename _TypeTranslator<T>::type>());
}

template<typename T>
int clsum(cl_mem buffer, 
          int size, 
          T& result) {
    typename _TypeTranslator<T>::type r;
    int status = clreduce(buffer, 
                          size, 
                          r, 
                          sum_reduce<typename _TypeTranslator<T>::type>());
    result = _TypeTranslator<T>::convert(r);

    return status;
}

#endif // _CL_REDUCE_H_