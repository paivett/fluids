#ifndef _MESH_H_
#define _MESH_H_

#include <cstddef>

typedef unsigned short mesh_index;

class Mesh {

    public:
        // Different drawing modes. Tells how the indices are organized
        // so that the renderer system knows how to interpret the data.
        enum Mode {
            POINTS,
            LINE_STRIP,
            LINE_LOOP,
            LINES,
            TRIANGLE_STRIP,
            TRIANGLE_FAN,
            TRIANGLES
        };

        virtual ~Mesh() {}

        /**
         * @brief Returns a pointer to a buffer of floats. Each vertex has x,y,z 
         * components, so the size of this buffer will be 3 * (num of vertices)
         */
        virtual const float* vertices() const = 0;

        /**
         * @brief Returns a pointer to a buffer of floats. Each normal has x,y,z 
         * components, so the size of this buffer will be 3 * (num of normals)
         */
        virtual const float* normals() const = 0;

        // Returns a shared ptr to the indices that define the mesh geometry
        virtual const mesh_index* mesh_indices() const = 0;

        // Returns the number of vertices of the mesh
        virtual std::size_t vertices_count() const = 0;

        // Returns the number of indices that define the mesh geometry
        // Note: its the size of the array returned by mesh_indices
        virtual std::size_t indices_count() const = 0;

        // Returns the mode in which the index data is organized
        virtual Mesh::Mode mode() const = 0;
};

#endif // _MESH_H_