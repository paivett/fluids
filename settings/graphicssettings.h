#ifndef _GRAPHICS_SETTINGS_H_
#define _GRAPHICS_SETTINGS_H_

#include <QColor>

enum RenderMethod {
    PARTICLES,
    SCREEN_SPACE
};

struct GraphicsSettings {
    QColor fluid_color;
    RenderMethod render_method;

    // Screen space fluid rendering filter iterations
    int sspace_filter_iterations;

    GraphicsSettings()
        : fluid_color(9, 97, 168),
          render_method(PARTICLES),
          sspace_filter_iterations(1)

    {}
};

#endif // _GRAPHICS_SETTINGS_H_
