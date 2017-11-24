#ifndef _MONAGHAN_KERNELS_H_
#define _MONAGHAN_KERNELS_H_

// Monaghan cubic spline kernel
// see "Smoothed particle hydrodynamics, 2005"

#define W_MONAGHAN_EVAL     (1.0f / (M_PI_F * CUBE(SUPPORT_RADIUS)))
#define W_MONAGHAN_GRAD     (-3.0f / (M_PI_F * FOURTH(SUPPORT_RADIUS)))
#define W_MONAGHAN_LAPL     (3.0f * W_MONAGHAN_GRAD)

inline float w_monaghan(float r_norm, float h) {
    if (r_norm <= h) {
        return W_MONAGHAN_EVAL * (1.0f - 1.5f * SQR(r_norm / h) + 3.0f/4.0f * CUBE(r_norm / h));
    }
    else if (r_norm <= (2.0f * h)) {
        return W_MONAGHAN_EVAL * 0.25f * CUBE(2.0f - r_norm / h);
    }
    else {
        return 0.0f;
    }
}


inline float4 grad_w_monaghan(float4 r, float r_norm, float h) {
    if (r_norm <= h) {
        return W_MONAGHAN_GRAD * r * INV(h) - (3.0f/4.0f) * (r_norm / SQR(h));
    }
    else if (r_norm <= (2.0f * h)) {
        return W_MONAGHAN_GRAD * r * 0.25f * INV(r_norm) * SQR(2.0f - r_norm / h);
    }
    else {
        return r * 0.0f;
    }
} 


inline float lapl_w_monaghan(float r_norm, float h) {
    if (r_norm <= h) {
        return W_MONAGHAN_LAPL * (INV(h) - 1.5f * (r_norm / SQR(h)));
    }
    else if (r_norm <= (2.0f * h)) {
        return W_MONAGHAN_LAPL * (INV(0.25f * r_norm) * SQR(2.0f - r_norm / h) - INV(r_norm) + r_norm / (4.0f * SQR(h)));
    }
    else {
        return 0.0f;
    }
}

#endif // _MONAGHAN_KERNELS_H_