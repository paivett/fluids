#include "quadmesh.h"

using namespace std;

QuadMesh::QuadMesh(float width, float height)
    : _vertices(12), _normals(12), _indices(6) {

    _vertices[0] = -width/2;
    _vertices[1] = height/2;
    _vertices[2] = 0;

    _vertices[3] = width/2;
    _vertices[4] = height/2;
    _vertices[5] = 0;

    _vertices[6] = width/2;
    _vertices[7] = -height/2;
    _vertices[8] = 0;

    _vertices[9]  = -width/2;
    _vertices[10] = -height/2;
    _vertices[11] = 0;

    _normals[0] = 0;
    _normals[1] = 0;
    _normals[2] = 1;

    _normals[3] = 0;
    _normals[4] = 0;
    _normals[5] = 1;

    _normals[6] = 0;
    _normals[7] = 0;
    _normals[8] = 1;

    _normals[9]  = 0;
    _normals[10] = 0;
    _normals[11] = 1;

    _indices[0] = 0;
    _indices[1] = 1;
    _indices[2] = 3;
    _indices[3] = 1;
    _indices[4] = 2;
    _indices[5] = 3;
}

QuadMesh::~QuadMesh() {
    
}

const float* QuadMesh::vertices() const {
    return _vertices.data();
}

const float* QuadMesh::normals() const {
    return _normals.data();
}

const mesh_index* QuadMesh::mesh_indices() const {
    return _indices.data();
}

size_t QuadMesh::vertices_count() const {
    return _vertices.size() / 3;
}

size_t QuadMesh::indices_count() const {
    return _indices.size();
}

Mesh::Mode QuadMesh::mode() const {
    return TRIANGLES;
}