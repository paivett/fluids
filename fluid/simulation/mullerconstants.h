#ifndef __MULLER_CONSTANTS__
#define __MULLER_CONSTANTS__

/**
 * @brief Muller kernel constants.
 * @see Particle-Based Fluid Simulation for Interactive Applications
 */
class MullerConstants {

    public:
        /**
         * @brief Constant of the default kernel
         * 
         * @param h Support radius
         * @return Kernel constant
         */
        static float default_eval(float h);
        
        /**
         * @brief Constant of the gradient of the default kernel
         * 
         * @param h Support radius
         * @return Gradient constant
         */
        static float default_grad(float h);
        
        /**
         * @brief Constant of the laplacian of the default kernel
         * 
         * @param h Support radius
         * @return Laplacian constant
         */
        static float default_lapl(float h);

        /**
         * @brief Constant of the pressure kernel
         * 
         * @param h Support radius
         * @return Kernel constant
         */
        static float pressure_eval(float h);
        
        /**
         * @brief Constant of the gradient of the pressure kernel
         * 
         * @param h Support radius
         * @return Gradient constant
         */
        static float pressure_grad(float h);
        
        /**
         * @brief Constant of the laplacian of the pressure kernel
         * 
         * @param h Support radius
         * @return Laplacian constant
         */
        static float pressure_lapl(float h);

        /**
         * @brief Constant of the viscosity kernel
         * 
         * @param h Support radius
         * @return Kernel constant
         */
        static float viscosity_eval(float h);
        
        /**
         * @brief Constant of the gradient of the viscosity kernel
         * 
         * @param h Support radius
         * @return Gradient constant
         */
        static float viscosity_grad(float h);
        
        /**
         * @brief Constant of the laplacian of the viscosity kernel
         * 
         * @param h Support radius
         * @return Laplacian constant
         */
        static float viscosity_lapl(float h);

};

#endif // __MULLER_CONSTANTS__