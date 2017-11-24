#ifndef _OBJ_MESH_H_
#define _OBJ_MESH_H_

#include "mesh.h"
#include <vector>
#include <string>


class ModelMesh : public Mesh {
    public:
        /**
         * @brief Create a new model mesh from a valid OBJ
         * 
         * @param model_filename The path to the OBJ filename.
         */
        ModelMesh(const std::string& model_filename);

        ~ModelMesh();

        /**
         * @brief Returns a pointer to a buffer of floats. Each vertex has x,y,z 
         * components, so the size of this buffer will be 3 * (num of vertices)
         */
        const float* vertices() const;

        /**
         * @brief Returns a pointer to a buffer of floats. Each normal has x,y,z 
         * components, so the size of this buffer will be 3 * (num of normals)
         */
        const float* normals() const;

        // Returns a shared ptr to the indices that define the mesh geometry
        const mesh_index* mesh_indices() const;

        // Returns the number of vertices of the mesh
        std::size_t vertices_count() const;

        // Returns the number of indices that define the mesh geometry
        // Note: its the size of the array returned by mesh_indices
        std::size_t indices_count() const;

        // Returns the mode in which the index data is organized
        Mesh::Mode mode() const;

    private:
        // Vertices of the sphere
        std::vector<float> _vertices;

        // Normals
        std::vector<float> _normals;

        // Pointer to the vertex indices
        std::vector<mesh_index> _indices;
};

#endif // _MODEL_MESH_H_
