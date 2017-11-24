#ifndef _CL_RANGE_H_
#define _CL_RANGE_H_

#include <vector>
#include <string>
#include "clenvironment.h"

class CLRange {

    public:
        CLRange(size_t x=0, size_t y=0, size_t z=0);

        size_t x() const;
        
        size_t y() const;
        
        size_t z() const;

        cl_uint dims() const;

        cl_uint count() const;

        const size_t* data() const;

        CLRange operator+(const CLRange& other) const;
        
        CLRange operator-(const CLRange& other) const;
        
        CLRange operator%(const CLRange& other) const;

    private:
        size_t _dims;

        std::vector<size_t> _size;
};

#endif // _CL_RANGE_H_