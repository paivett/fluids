#ifndef _KERNELS_H_
#define _KERNELS_H_

#include "macros.h"
#include "muller_kernels.h"
#include "monaghan_kernels.h"

#ifdef USE_MULLER_KERNELS

    #define W_DEFAULT(r_norm, h)             W_POLY6(r_norm, h)
    #define GRAD_W_DEFAULT(r, r_norm, h)     GRAD_W_POLY6(r, r_norm, h)
    #define LAPL_W_DEFAULT(r_norm, h)        LAPL_W_POLY6(r_norm, h)

    #define W_PRESSURE(r_norm, h)            W_SPIKY(r_norm, h)
    #define GRAD_W_PRESSURE(r, r_norm, h)    GRAD_W_SPIKY(r, r_norm, h)
    #define LAPL_W_PRESSURE(r_norm, h)       LAPL_W_SPIKY(r_norm, h)

    #define W_VISCOSITY(r_norm, h)           W_VISC(r_norm, h)
    #define GRAD_W_VISCOSITY(r, r_norm, h)   GRAD_W_VISC(r, r_norm, h)
    #define LAPL_W_VISCOSITY(r_norm, h)      LAPL_W_VISC(r_norm, h)

#else

    #define W_DEFAULT(r_norm, h)             w_monaghan(r_norm, h)
    #define GRAD_W_DEFAULT(r, r_norm, h)     grad_w_monaghan(r, r_norm, h)
    #define LAPL_W_DEFAULT(r_norm, h)        lapl_w_monaghan(r_norm, h)

    #define W_PRESSURE(r_norm, h)            w_monaghan(r_norm, h)
    #define GRAD_W_PRESSURE(r, r_norm, h)    grad_w_monaghan(r, r_norm, h)
    #define LAPL_W_PRESSURE(r_norm, h)       lapl_w_monaghan(r_norm, h)

    #define W_VISC(r_norm, h)                w_monaghan(r_norm, h)
    #define GRAD_W_VISCOSITY(r, r_norm, h)   grad_w_monaghan(r, r_norm, h)
    #define LAPL_W_VISCOSITY(r_norm, h)      lapl_w_monaghan(r_norm, h)

#endif

#endif // _KERNELS_H_