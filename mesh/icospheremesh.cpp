#include "icospheremesh.h"
#include <cmath>
#include <LinearMath/btVector3.h>

#include <iostream>

using namespace std;

// @see http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html

IcoSphereMesh::IcoSphereMesh(float radius, unsigned int level) {
    // At the beginning, the icosphere has 12 vertices
    vector<btVector3> tmp_vertices(12);
    // And 20 faces, its an icosahedron
    _indices.resize(20*3);

    // Complete the vertices first
    auto t = (1.0 + sqrt(5.0))*0.5;

    tmp_vertices[0]  = btVector3(-1,  t,  0);
    tmp_vertices[1]  = btVector3( 1,  t,  0);
    tmp_vertices[2]  = btVector3(-1, -t,  0);
    tmp_vertices[3]  = btVector3( 1, -t,  0);

    tmp_vertices[4]  = btVector3( 0, -1,  t);
    tmp_vertices[5]  = btVector3( 0,  1,  t);
    tmp_vertices[6]  = btVector3( 0, -1, -t);
    tmp_vertices[7]  = btVector3( 0,  1, -t);

    tmp_vertices[8]  = btVector3( t,  0, -1);
    tmp_vertices[9]  = btVector3( t,  0,  1);
    tmp_vertices[10] = btVector3(-t,  0, -1);
    tmp_vertices[11] = btVector3(-t,  0,  1);

    // Now lets add the 20 faces
    _indices[0]  = 0; _indices[1]  = 11;  _indices[2]  = 5;
    _indices[3]  = 0; _indices[4]  = 5;   _indices[5]  = 1;
    _indices[6]  = 0; _indices[7]  = 1;   _indices[8]  = 7;
    _indices[9]  = 0; _indices[10] = 7;   _indices[11] = 10;
    _indices[12] = 0; _indices[13] = 10;  _indices[14] = 11;

    _indices[15] = 1;  _indices[16] = 5;  _indices[17] = 9;
    _indices[18] = 5;  _indices[19] = 11; _indices[20] = 4;
    _indices[21] = 11; _indices[22] = 10; _indices[23] = 2;
    _indices[24] = 10; _indices[25] = 7;  _indices[26] = 6;
    _indices[27] = 7;  _indices[28] = 1;  _indices[29] = 8;

    _indices[30] = 3; _indices[31] = 9; _indices[32] = 4;
    _indices[33] = 3; _indices[34] = 4; _indices[35] = 2;
    _indices[36] = 3; _indices[37] = 2; _indices[38] = 6;
    _indices[39] = 3; _indices[40] = 6; _indices[41] = 8;
    _indices[42] = 3; _indices[43] = 8; _indices[44] = 9;

    _indices[45] = 4; _indices[46] = 9; _indices[47] = 5;
    _indices[48] = 2; _indices[49] = 4; _indices[50] = 11;
    _indices[51] = 6; _indices[52] = 2; _indices[53] = 10;
    _indices[54] = 8; _indices[55] = 6; _indices[56] = 7;
    _indices[57] = 9; _indices[58] = 8; _indices[59] = 1;

    // Now refine the sphere
    for (unsigned i=0; i<level; ++i) {
        //auto indices_aux = _indices;
        vector<mesh_index> indices_aux;
        auto vertices_aux = tmp_vertices;

        // For each face, we add 4 new faces
        for (size_t k=0; k < _indices.size(); k+=3) {
            auto idx_1 = _indices[k];
            auto idx_2 = _indices[k+1];
            auto idx_3 = _indices[k+2];

            auto v1 = tmp_vertices[idx_1];
            auto v2 = tmp_vertices[idx_2];
            auto v3 = tmp_vertices[idx_3];

            // these are the new vertices
            auto v1_2 = ((v1 + v2) * 0.5);
            auto v2_3 = ((v2 + v3) * 0.5);
            auto v3_1 = ((v3 + v1) * 0.5);

            vertices_aux.push_back(v1_2);
            auto idx_1_2 = vertices_aux.size()-1;

            vertices_aux.push_back(v2_3);
            auto idx_2_3 = vertices_aux.size()-1;

            vertices_aux.push_back(v3_1);
            auto idx_3_1 = vertices_aux.size()-1;

            indices_aux.push_back(idx_1);
            indices_aux.push_back(idx_1_2);
            indices_aux.push_back(idx_3_1);

            indices_aux.push_back(idx_2);
            indices_aux.push_back(idx_2_3);
            indices_aux.push_back(idx_1_2);

            indices_aux.push_back(idx_3);
            indices_aux.push_back(idx_3_1);
            indices_aux.push_back(idx_2_3);

            indices_aux.push_back(idx_1_2);
            indices_aux.push_back(idx_2_3);
            indices_aux.push_back(idx_3_1);
        }

        _indices = indices_aux;
        tmp_vertices = vertices_aux;
    }

    // Set up normals, and adjust icosphere radius
    for (size_t i=0; i<tmp_vertices.size(); ++i) {
        auto v = tmp_vertices[i].normalized();
        auto v2 = v * radius;
        
        _normals.push_back(v.x());
        _normals.push_back(v.y());
        _normals.push_back(v.z());
        _vertices.push_back(v2.x());
        _vertices.push_back(v2.y());
        _vertices.push_back(v2.z());
    }
}

const float* IcoSphereMesh::vertices() const {
    return _vertices.data();
}

const float* IcoSphereMesh::normals() const {
    return _normals.data();
}

const mesh_index* IcoSphereMesh::mesh_indices() const {
    return _indices.data();
}

size_t IcoSphereMesh::vertices_count() const {
    return _vertices.size() / 3;
}

size_t IcoSphereMesh::indices_count() const {
    return _indices.size();
}

Mesh::Mode IcoSphereMesh::mode() const {
    return TRIANGLES;
}