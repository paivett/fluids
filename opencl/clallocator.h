/**
 *  @file clallocator.h
 *  @brief Contains the declaration of the CLAllocator class.
 *
 *  This contains the declaration of the class CLAllocator, which facilitates
 *  the allocation of device buffers.
 *
 *  @author Santiago Daniel Pivetta
 */

#ifndef _CL_ALLOCATOR_H_
#define _CL_ALLOCATOR_H_

#include <GL/gl.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <vector>
#include "clerror.h"
#include "opengl/openglfunctions.h"

class CLAllocator {
    public:
        
        static cl_mem alloc_1d_image_from_buff(size_t size, cl_channel_order ch_order, cl_mem buffer) {
            cl_int err;
            
            cl_image_format fmt;
            fmt.image_channel_order = ch_order;
            fmt.image_channel_data_type = CL_FLOAT;

            cl_image_desc desc;
            desc.image_type = CL_MEM_OBJECT_IMAGE1D_BUFFER;
            desc.image_width = size;
            //desc.image_array_size = size;
            desc.image_row_pitch = 0;
            desc.image_slice_pitch = 0;
            desc.num_mip_levels = 0;
            desc.num_samples = 0;
            desc.buffer = buffer;

            cl_mem image = clCreateImage(CLEnvironment::context(),
                                         CL_MEM_READ_ONLY,
                                         &fmt,
                                         &desc,
                                         nullptr,
                                         &err);

            CLError::check(err);

            return image;
        }
        
            
        
        /**
         * @brief Allocates a new cl_mem on the device
         * @details Allocates a new cl_mem buffer on the device. The
         * size of the buffer is size * sizeof(T).
         *
         * @param size Number of elements of the buffer
         * @param mem_flag Memory flag modifier
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         * @return An instance of cl_mem.
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static cl_mem alloc_buffer(size_t size, cl_mem_flags mem_flag=CL_MEM_READ_WRITE) {
            cl_int err;
            cl_mem buffer = clCreateBuffer(CLEnvironment::context(),
                                           mem_flag,
                                           size * sizeof(T),
                                           nullptr,
                                           &err);
            CLError::check(err);

            return buffer;
        }

        /**
         * @brief Allocates, and initializes, a new cl_mem on the device
         * @details Allocates a new cl_mem buffer on the device. The size of the
         *          buffer is size * sizeof(T). The buffer is filled with the
         *          initialization data provided by the values vector.
         *
         * @param size Number of elements of the buffer
         * @param values The vector with the initial values
         * @param mem_flag Memory flag modifier
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         * @return An instance of cl_mem.
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static cl_mem alloc_buffer(size_t size, const std::vector<T>& values, cl_mem_flags mem_flag=CL_MEM_READ_WRITE) {
            cl_mem buffer = CLAllocator::alloc_buffer<T>(size, mem_flag);

            cl_int err = clEnqueueWriteBuffer(CLEnvironment::queue(),
                                              buffer,
                                              CL_TRUE,
                                              0,
                                              size * sizeof(T),
                                              values.data(),
                                              0,
                                              nullptr,
                                              nullptr);
            CLError::check(err);

            return buffer;
        }

        /**
         * @brief Allocates, and initializes, a new cl_mem on the device
         * @details Allocates a new cl_mem buffer on the device. The size of the
         *          buffer is size * sizeof(T). The buffer is filled with the
         *          value by the default_value parameter.
         *
         * @param size Number of elements of the buffer
         * @param default_value The initial value of all elements of the buffer
         * @param mem_flag Memory flag modifier
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         * @return An instance of cl_mem.
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static cl_mem alloc_buffer(size_t size, T default_value, cl_mem_flags mem_flag=CL_MEM_READ_WRITE) {
            std::vector<T> values(size, default_value);
            cl_mem buffer = CLAllocator::alloc_buffer<T>(size, values, mem_flag);

            return buffer;
        }

        /**
         * @brief Releases a device buffer
         * @details This function releases a cl_mem buffer. There is no
         *          validation wether the buffer is valid or not.
         *
         * @param buffer The buffer to be released
         *
         * @throws CLError if the buffer could not be allocated.
         */
        static void release_buffer(cl_mem buffer) {
            if (buffer) {
                cl_int err = clReleaseMemObject(buffer);
                CLError::check(err);
            }
        }

        /**
         * @brief Allocates a new device buffer, shared with OpenGL
         * @details Uses the OpenCL-OpenGL interop api to allocate a new,
         *          shared buffer. The new buffer will have
         *          size * sizeof(T) bytes
         *
         * @param size The number of elements of the buffer
         * @param vbo The OpenGL Vertex Buffer Object id
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         * @return An instance of cl_mem.
         *
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static cl_mem alloc_gl_buffer(size_t size, GLuint vbo) {
            cl_int err;
            auto& gl = OpenGLFunctions::getFunctions();

            // Resize the OpenGL VBO's
            gl.glBindBuffer(GL_ARRAY_BUFFER, vbo);
            gl.glBufferData(GL_ARRAY_BUFFER,
                            size * sizeof(T),
                            nullptr,
                            GL_DYNAMIC_DRAW);

            cl_mem buffer = clCreateFromGLBuffer(CLEnvironment::context(),
                                                 CL_MEM_READ_WRITE,
                                                 vbo,
                                                 &err);
            CLError::check(err);

            return buffer;
        }

        /**
         * @brief Copies a buffer
         * @details Copies the whole buffer to another one. The copy begins at
         *          offset 0.
         *
         * @param src Source device buffer.
         * @param dst Destination device buffer.
         * @param size Number of elements to be copied.
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         *
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static void copy_full_buffer(cl_mem src, cl_mem dst, size_t size) {
            cl_int err = clEnqueueCopyBuffer(CLEnvironment::queue(),
                                             src,
                                             dst,
                                             0,
                                             0,
                                             size * sizeof(T),
                                             0,
                                             nullptr,
                                             nullptr);
            CLError::check(err);
        }

        /**
         * @brief Downloads a buffer to a std::vector
         * @details Downloads a full device buffer to a host std::vector,
         *          starting at offset 0. The size of the buffer must be the
         *          same as the number of elements of the vector.
         *
         * @param src Source device buffer.
         * @param dst Destination host vector.
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         *
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static void download_buffer(cl_mem src, std::vector<T>& dst) {
            cl_int err = clEnqueueReadBuffer(CLEnvironment::queue(),
                                             src,
                                             CL_TRUE,
                                             0,
                                             dst.size() * sizeof(T),
                                             dst.data(),
                                             0,
                                             nullptr,
                                             nullptr);
            CLError::check(err);
        }

        /**
         * @brief Downloads a buffer to a std::vector
         * @details Downloads a full device buffer to a host std::vector,
         *          starting at offset 0.
         *
         * @param src Source device buffer.
         * @param size The number of elements of the buffer. (Each element is
         *             of sizeof(T) bytes)
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         * @return The host copy of the device buffer.
         *
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static std::vector<T> download_buffer(cl_mem src, size_t size) {
            std::vector<T> dst(size);
            CLAllocator::download_buffer(src, dst);
            return dst;
        }

        /**
         * @brief Uploads the contents of a local buffer to a devide buffer
         * @details Uploads a full host std::vector to a device buffer. Device
         *          buffer must have at least the same size of the host buffer.
         *
         * @param src Source host buffer.
         * @param dst Destination device buffer.
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         *
         * @throws CLError if the write operation fails.
         */
        template<class T>
        static void upload_to_buffer(std::vector<T> src, cl_mem dst) {
            cl_int err = clEnqueueWriteBuffer(CLEnvironment::queue(),
                                              dst,
                                              CL_TRUE,
                                              0,
                                              src.size() * sizeof(T),
                                              src.data(),
                                              0,
                                              nullptr,
                                              nullptr);
            CLError::check(err);
        }

        /**
         * @brief Uploads the contents of a local buffer to a device buffer
         * @details Uploads a full host std::vector to a device buffer. Device
         *          buffer must have at least the same size of the host buffer.
         *          Device buffer is a shared buffer with OpenGL.
         *
         * @param src Source host buffer.
         * @param dst Destination CL-GL device buffer.
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         *
         * @throws CLError if the write operation fails.
         */
        template<class T>
        static void upload_to_gl_buffer(const std::vector<T>& src, cl_mem dst) {
            OpenGLFunctions::getFunctions().glFinish();
            cl_int err = clEnqueueAcquireGLObjects(CLEnvironment::queue(),
                                                   1,
                                                   &dst,
                                                   0,
                                                   nullptr,
                                                   nullptr);
            CLError::check(err);
            CLAllocator::upload_to_buffer(src, dst);
            err = clEnqueueReleaseGLObjects(CLEnvironment::queue(),
                                            1,
                                            &dst,
                                            0,
                                            nullptr,
                                            nullptr);
            CLError::check(err);
        }

        /**
         * @brief Fills a buffer with a given value
         * @details Fills a buffer with a given constant value.
         * 
         * @param buffer Source device buffer.
         * @param val Value to fill the buffer with.
         * @param size The number of elements of the device buffer (not the number of bytes).
         * @tparam T Type of each element of the buffer. The types must be
         *           compliant with the types OpenCL provides.
         *           
         * @throws CLError if the buffer could not be allocated.
         */
        template<class T>
        static cl_int fill_buffer(cl_mem buffer, const T& val, size_t size, cl_event* event=nullptr) {
            cl_int err =  clEnqueueFillBuffer(CLEnvironment::queue(),
                                              buffer,
                                              &val,
                                              sizeof(val),
                                              0,
                                              sizeof(val) * size,
                                              0,
                                              nullptr,
                                              event);
            return err;
        }

        /**
         * @brief Locks GL shared buffers
         * 
         * @param buffers A vector of buffers to lock
         */
        static void lock_gl_buffers(const std::vector<cl_mem>& buffers) {
            OpenGLFunctions::getFunctions().glFinish();
            cl_int err = clEnqueueAcquireGLObjects(CLEnvironment::queue(),
                                                   buffers.size(),
                                                   buffers.data(),
                                                   0,
                                                   nullptr,
                                                   nullptr);
            CLError::check(err);
        }

        /**
         * @brief Unlocks GL shared buffers
         * 
         * @param buffers A vector of buffers to unlock
         */
        static void unlock_gl_buffers(const std::vector<cl_mem>& buffers) {
            cl_int err = clEnqueueReleaseGLObjects(CLEnvironment::queue(),
                                           buffers.size(),
                                           buffers.data(),
                                           0,
                                           nullptr,
                                           nullptr);
            CLError::check(err);
        }

    private:
        CLAllocator();
        CLAllocator(const CLAllocator& e);
        CLAllocator& operator=(const CLAllocator& e);
};

#endif // _CL_ALLOCATOR_H_
