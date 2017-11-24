#define TINYOBJLOADER_IMPLEMENTATION

#include "modelmesh.h"
#include "external/tinyobj/tiny_obj_loader.h"
#include <algorithm>
#include <iostream>
#include <map>

using namespace std;

/**
 * This implementation uses tinyobj
 * @see https://github.com/syoyo/tinyobjloader
 */

ModelMesh::ModelMesh(const string& model_filename) {
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;

    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, model_filename.c_str());

    if (!err.empty()) { // `err` may contain warning message.
        cerr << err << std::endl;
    }

    if (!ret) {
        string msg = "Could not load model: ";
        msg += model_filename;
        throw runtime_error(msg);
    }

    // Since the OBJ may contain a different number of normals and vertices, 
    // I use this map to keep track of already seen pairs of vertex/normal
    // If a vertex has already been loaded, but with a different normal, then
    // we must load a copy of that vertex to use the new normal
    // This is how OpenGL Array Buffer expects things
    map<pair<int, int>, int> loaded_indices;

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = shapes[s].mesh.num_face_vertices[f];

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // Access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                
                auto p = pair<int, int>(idx.vertex_index, idx.normal_index);
                auto i = loaded_indices.find(p);

                if (i != loaded_indices.end()) {
                    // The vertex/normal combination has already been loaded
                    _indices.push_back(i->second);
                }
                else {
                    // This vertex/normal combination has not been loaded yet
                    float vx = attrib.vertices[3*idx.vertex_index+0];
                    float vy = attrib.vertices[3*idx.vertex_index+1];
                    float vz = attrib.vertices[3*idx.vertex_index+2];
                    float nx = attrib.normals[3*idx.normal_index+0];
                    float ny = attrib.normals[3*idx.normal_index+1];
                    float nz = attrib.normals[3*idx.normal_index+2];

                    int new_idx = _vertices.size()/3;
                    
                    _vertices.push_back(vx);
                    _vertices.push_back(vy);
                    _vertices.push_back(vz);
                    _normals.push_back(nx);
                    _normals.push_back(ny);
                    _normals.push_back(nz);
                    _indices.push_back(new_idx);
                    loaded_indices[p] = new_idx;
                }
            }

            index_offset += fv;
        }
    }   
}

ModelMesh::~ModelMesh() {

}

const float* ModelMesh::vertices() const {
    return _vertices.data();
}

const float* ModelMesh::normals() const {
    return _normals.data();
}

const mesh_index* ModelMesh::mesh_indices() const {
    return _indices.data();
}

size_t ModelMesh::vertices_count() const {
    return _vertices.size() / 3;
}

size_t ModelMesh::indices_count() const {
    return _indices.size();
}

Mesh::Mode ModelMesh::mode() const {
    return TRIANGLES;
}