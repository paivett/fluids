#include "clrange.h"
#include "external/json/json11.hpp"

using namespace std;

CLRange::CLRange(size_t x, size_t y, size_t z) : _size(3, 0) {
    _size[0] = x;
    _size[1] = (y > 0) ? y : 1;
    _size[2] = (z > 0) ? z : 1;

    _dims = 1 + ((y > 1) ? 1 : 0) + ((z > 1) ? 1 : 0);
}

cl_uint CLRange::dims() const {
    return _dims;
}

cl_uint CLRange::count() const {
    return _size[0] * _size[1] * _size[2];
}

const size_t* CLRange::data() const {
    return _size.data();
}

CLRange CLRange::operator+(const CLRange &other) const {
    CLRange res(*this);

    for (size_t i=0; i < _dims; ++i) {
        res._size[i] += other._size[i];
    }

    return res;
}

CLRange CLRange::operator-(const CLRange &other) const {
    CLRange res(*this);

    for (size_t i=0; i < _dims; ++i) {
        res._size[i] -= other._size[i];
    }

    return res;
}

CLRange CLRange::operator%(const CLRange &other) const {
    CLRange res(*this);

    for (size_t i=0; i < _dims; ++i) {
        res._size[i] = _size[i] % other._size[i];
    }

    return res;
}

size_t CLRange::x() const {
    return _size[0];
}

size_t CLRange::y() const {
    return _size[1];
}

size_t CLRange::z() const {
    return _size[2];
}