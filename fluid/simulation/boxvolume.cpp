#include "boxvolume.h"

using namespace std;

BoxVolume::BoxVolume(const btVector3& size, const btVector3& center) : 
_size(size), 
_center(center) {

}

vector<cl_float4> BoxVolume::particles(float particle_radius) {
    vector<cl_float4> particles;

    float particle_spacing = 2 * particle_radius; 
    float extent_x = _size.getX();
    float extent_y = _size.getY();
    float extent_z = _size.getZ();
    int ppside_x = ceil(extent_x / particle_spacing); //nx
    int ppside_y = ceil(extent_y / particle_spacing);
    int ppside_z = ceil(extent_z / particle_spacing);
    float delta_x = extent_x / ppside_x; //d
    float delta_y = extent_y / ppside_y;
    float delta_z = extent_z / ppside_z;

    for (int j=0; j<ppside_y; ++j) {
        for (int i=0; i<ppside_x; ++i) {
            for (int k=0; k<ppside_z; ++k) {
                // Initialize positions
                cl_float4 p;
                p.s[0] = i * delta_x + _center.getX() - _size.getX() / 2.0;
                p.s[1] = j * delta_y + _center.getY() - _size.getY() / 2.0;
                p.s[2] = k * delta_z + _center.getZ() - _size.getZ() / 2.0;
                p.s[3] = 1; // Homogeneous system
                particles.push_back(p);
            }
        }
    }

    return particles;
}
