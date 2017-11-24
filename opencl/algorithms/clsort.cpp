#include "clsort.h"
#include <unordered_map>

clogs::Type __clType_2_clogsType(std::type_index t_idx) {
    std::unordered_map<std::type_index, clogs::Type> tmap;
    tmap[typeid(cl_uchar)] = clogs::Type(clogs::TYPE_UCHAR);
    tmap[typeid(cl_char)] = clogs::Type(clogs::TYPE_CHAR);
    tmap[typeid(cl_short)] = clogs::Type(clogs::TYPE_SHORT);
    tmap[typeid(cl_ushort)] = clogs::Type(clogs::TYPE_USHORT);
    tmap[typeid(cl_int)] = clogs::Type(clogs::TYPE_INT);
    tmap[typeid(cl_uint)] = clogs::Type(clogs::TYPE_UINT);
    tmap[typeid(cl_long)] = clogs::Type(clogs::TYPE_LONG);
    tmap[typeid(cl_ulong)] = clogs::Type(clogs::TYPE_ULONG);
    tmap[typeid(cl_half)] = clogs::Type(clogs::TYPE_HALF);
    tmap[typeid(cl_float)] = clogs::Type(clogs::TYPE_FLOAT);
    tmap[typeid(cl_double)] = clogs::Type(clogs::TYPE_DOUBLE);

    return tmap[t_idx];
}