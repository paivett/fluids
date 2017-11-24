#include "boxmesh.h"
#include <algorithm>

BoxMesh::BoxMesh(float width, float height, float depth) {
    auto x = width/2;
    auto y = height/2;
    auto z = depth/2;

    _vertices = {
        x, y,  z,
        x, y, -z,
        -x, y, -z,
        -x, y, z,
        x, -y, z,
        x, -y, -z,
        -x, -y, -z,
        -x, -y, z,
         x, -y, z,
         x,  y, z,
        -x,  y, z,
        -x, -y, z,
         x, -y, -z,
         x,  y, -z,
        -x,  y, -z,
        -x, -y, -z,
         x, -y, -z,
         x,  y, -z,
         x,  y,  z,
         x, -y,  z,
        -x, -y, -z,
        -x,  y, -z,
        -x,  y,  z,
        -x, -y,  z
    };

    _normals = {
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        0, 1, 0,
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
        0, -1, 0,
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        0, 0, 1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        0, 0, -1,
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
        -1, 0, 0,
    };

    _indices = {
        0, 1, 2,
        2, 3, 0,
        6, 4, 5,
        4, 7, 6,
        8, 9, 10,
        10, 11, 8,
        14, 13, 12,
        12, 15, 14,
        16, 17, 18,
        18, 19, 16,
        22, 21, 20,
        20, 23, 22
    };
}

BoxMesh::~BoxMesh() {

}

const float* BoxMesh::vertices() const {
    return _vertices.data();
}

const float* BoxMesh::normals() const {
    return _normals.data();
}

const mesh_index* BoxMesh::mesh_indices() const {
    return _indices.data();
}

size_t BoxMesh::vertices_count() const {
    return _vertices.size() / 3;
}

size_t BoxMesh::indices_count() const {
    return _indices.size();
}

Mesh::Mode BoxMesh::mode() const {
    return TRIANGLES;
}