#include "wireboxmesh.h"
#include <LinearMath/btVector3.h>

using namespace std;

WireBoxMesh::WireBoxMesh(float width, float height, float depth)
    : _indices(24) {
    
    float x = width/2;
    float y = height/2;
    float z = depth/2;

    _vertices.push_back(x);_vertices.push_back(-y);_vertices.push_back(z);
    _vertices.push_back( x);_vertices.push_back(-y);_vertices.push_back(-z);
    _vertices.push_back(-x);_vertices.push_back(-y);_vertices.push_back(-z);
    _vertices.push_back(-x);_vertices.push_back(-y);_vertices.push_back( z);
    _vertices.push_back( x);_vertices.push_back( y);_vertices.push_back( z);
    _vertices.push_back( x);_vertices.push_back( y);_vertices.push_back(-z);
    _vertices.push_back(-x);_vertices.push_back( y);_vertices.push_back(-z);
    _vertices.push_back(-x);_vertices.push_back( y);_vertices.push_back( z);

    _indices[0]  = 0; _indices[1]  = 1;
    _indices[2]  = 1; _indices[3]  = 2;
    _indices[4]  = 2; _indices[5]  = 3;
    _indices[6]  = 3; _indices[7]  = 0;
    
    _indices[8]  = 4; _indices[9]  = 5;
    _indices[10] = 5; _indices[11] = 6;
    _indices[12] = 6; _indices[13] = 7;
    _indices[14] = 7; _indices[15] = 4;
    
    _indices[16] = 0; _indices[17] = 4;
    _indices[18] = 1; _indices[19] = 5;
    _indices[20] = 2; _indices[21] = 6;
    _indices[22] = 3; _indices[23] = 7;

    _normals.resize(_vertices.size());
    for (size_t i=0; i < _vertices.size()/3; ++i) {
        auto x = _vertices[3*i];
        auto y = _vertices[3*i + 1];
        auto z = _vertices[3*i + 2];
        btVector3 v(x, y, z);
        v = v.normalized();
        _normals.push_back(v.x());
        _normals.push_back(v.y());
        _normals.push_back(v.z());
    }
}

WireBoxMesh::~WireBoxMesh() {

}

const float* WireBoxMesh::vertices() const {
    return _vertices.data();
}

const float* WireBoxMesh::normals() const {
    return _normals.data();
}

const mesh_index* WireBoxMesh::mesh_indices() const {
    return _indices.data();
}

size_t WireBoxMesh::vertices_count() const {
    return _vertices.size() / 3;
}

size_t WireBoxMesh::indices_count() const {
    return _indices.size();
}

Mesh::Mode WireBoxMesh::mode() const {
    return LINES;
}