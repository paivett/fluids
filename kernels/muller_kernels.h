#ifndef _MULLER_KERNELS_H_
#define _MULLER_KERNELS_H_

#include "macros.h"

// Muller-Fish kernels
// see "Particle-based fluid simulation for interactive applications, 2003"

#define W_POLY6(r_norm, h)          (CUBE(SQR(h) - SQR(r_norm)))
#define GRAD_W_POLY6(r, r_norm, h)  (r * SQR(SQR(h) - SQR(r_norm)))
#define LAPL_W_POLY6(r_norm, h)     ()

#define W_SPIKY(r_norm, h)          (CUBE(h - r_norm))
#define GRAD_W_SPIKY(r, r_norm, h)  (r * INV(r_norm) * SQR(h - r_norm))
#define LAPL_W_SPIKY(r_norm, h)     (INV(r_norm) * (h - r_norm) * (h - 2.0f * r_norm))

#define W_VISC(r_norm, h)          ((-0.5f * CUBE(r_norm/h)) + SQR(r_norm/h) + (h / (2.0f * r_norm)) - 1.0f)
#define GRAD_W_VISC(r, r_norm, h)  (r * (-1.5f * r_norm / CUBE(h) + 2.0f/SQR(h) - 0.5f * h / CUBE(r_norm)))
#define LAPL_W_VISC(r_norm, h)     (h - r_norm)

#endif // _MULLER_KERNELS_H_