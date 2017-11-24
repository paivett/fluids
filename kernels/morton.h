#ifndef _MORTON_CL_H_
#define _MORTON_CL_H_

// Given a 32 bit morton code, unpacks it to 2 16-bit indices
// @param morton The 32-bit morton code
// @param index1 A pointer to the index 1 (x)
// @param index2 A pointer to the index 2 (y) 
inline void morton_2d_decode(const uint code, uint* index1, uint* index2) {
    uint value1 = code;
    uint value2 = ( value1 >> 1 );
    value1 &= 0x55555555;
    value2 &= 0x55555555;
    value1 |= ( value1 >> 1 );
    value2 |= ( value2 >> 1 );
    value1 &= 0x33333333;
    value2 &= 0x33333333;
    value1 |= ( value1 >> 2 );
    value2 |= ( value2 >> 2 );
    value1 &= 0x0f0f0f0f;
    value2 &= 0x0f0f0f0f;
    value1 |= ( value1 >> 4 );
    value2 |= ( value2 >> 4 );
    value1 &= 0x00ff00ff;
    value2 &= 0x00ff00ff;
    value1 |= ( value1 >> 8 );
    value2 |= ( value2 >> 8 );
    value1 &= 0x0000ffff;
    value2 &= 0x0000ffff;
    *index1 = value1;
    *index2 = value2;
} 

// Function to seperate bits from a given 32-bit integer 3 positions apart
// using "magic bits". For further speed consider using table look up
// Note: this method was taken from
// http://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/
inline uint split_by_3(uint a) {
    uint x = a & 0x1fffff; // we only look at the first 21 bits
    x = (x | x << 32) & 0x1f00000000ffff;  // shift left 32 bits, OR with self, and 00011111000000000000000000000000000000001111111111111111
    x = (x | x << 16) & 0x1f0000ff0000ff;  // shift left 32 bits, OR with self, and 00011111000000000000000011111111000000000000000011111111
    x = (x | x << 8) & 0x100f00f00f00f00f; // shift left 32 bits, OR with self, and 0001000000001111000000001111000000001111000000001111000000000000
    x = (x | x << 4) & 0x10c30c30c30c30c3; // shift left 32 bits, OR with self, and 0001000011000011000011000011000011000011000011000011000100000000
    x = (x | x << 2) & 0x1249249249249249;
    return x;
}

// Given a 3d position in space, returns the morton (space filling curve)
// code
inline uint morton_3d_encode(uint x, uint y, uint z) {
    uint hx = split_by_3(x);
    uint hy = split_by_3(y) << 1;
    uint hz = split_by_3(z) << 2;
    hx = hx | hy;
    hx = hx | hz;
    return hx;
}

#endif // _MORTON_CL_H_