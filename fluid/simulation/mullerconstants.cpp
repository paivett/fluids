#include "mullerconstants.h"
#include <cmath>

float MullerConstants::default_eval(float h) {
    return 365.0f / (64.0f * M_PI * pow(h, 9.0f));
}

float MullerConstants::default_grad(float h) {
    return -945.0f / (32.0f * M_PI * pow(h, 9.0f));
}

float MullerConstants::default_lapl(float h) {
    return default_grad(h);
}

float MullerConstants::pressure_eval(float h) {
    return 15.0f / (64.0f * M_PI * pow(h, 9.0f));
}

float MullerConstants::pressure_grad(float h) {
    return -45.0f / (M_PI * pow(h, 6.0f));
}

float MullerConstants::pressure_lapl(float h) {
    return -90.0f / (M_PI * pow(h, 6.0f));
}

float MullerConstants::viscosity_eval(float h) {
    return 15.0f / (2.0f * M_PI * pow(h, 3.0));
}

float MullerConstants::viscosity_grad(float h) {
    return viscosity_eval(h);
}

float MullerConstants::viscosity_lapl(float h) {
    return 45.0f / (M_PI * pow(h, 6.0f));
}
