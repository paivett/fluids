/**
 * @file   surface_tension.h
 * @Author santiago.pivetta@gmail.com
 * @brief  SPH surface tension model functions
 *
 * These are some functions that implement the surface tension 
 * model described in the paper 
 * "Versatile surface tension and adhesion for SPH fluids" by 
 * Nadir Akinci, Gizem Akinci and Matthias Teschner.
 */

#ifndef _SURFACE_TENSION_MODEL_H_ 
#define _SURFACE_TENSION_MODEL_H_

/**
 * @brief Returns the correction factor of the surface tension model.
 * 
 * @param rest_density Rest density of the fluid.
 * @param di Density of the i-th current particle.
 * @param dj Density of the j-th neighbour particle.
 * @return The correction factor.
 */
inline float st_correction_factor(float rest_density, float di, float dj) {
    return 2.0f * rest_density / (di + dj);
}

/**
 * @brief 3D SPH Spline
 * 
 * @param r Distance between particles.
 * @param h Support radius.
 * @param st_kernel_constant Spline constant, computed outside for precision.
 * @return Spline 
 */
inline float st_kernel(float r, float h, float st_kernel_constant) {
    if( r <= 0.5f * h ) {
        float t1 = (h - r);
        float t2 = (t1 * t1 * t1) * (r * r * r);
        return 2.0f * t2 - st_kernel_constant;
    }
    else if( r <= h ) {
        float t1 = (h - r);
        return (t1 * t1 * t1) * (r * r * r);
    }
    else {
        return 0.0f;
    }
}

/**
 * @brief Partial computation of the local curvature force.
 * @details Partially computes the local curvature force of the surface 
 *          tension model. The value returned is not multiplied by the kernel 
 *          constant nor the particle mass.
 * 
 * @param rest_density Rest density of the fluid.
 * @param di Local density.
 * @param dj Neighbour density.
 * @param ni Local normal.
 * @param nj Neighbour normal.
 * @return The local curvature force.
 */
 float4 st_local_curvature(float rest_density, 
                                 float di, 
                                 float dj, 
                                 float4* ni, 
                                 float4* nj) {
    return (*ni - *nj) * 2.0f * (rest_density / (di + dj));
}


 float4 st_local_cohesion(float4* r,
                                float r_norm,
                                float h,
                                float rest_density,
                                float di, 
                                float dj,
                                float st_kernel_term) {
    return normalize(*r) * st_kernel(r_norm, h, st_kernel_term) * 2.0f * (rest_density / (di + dj));
}

#endif // _SURFACE_TENSION_MODEL_H_
