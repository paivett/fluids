#include "material.h"
#include <vector>
#include <random>

Material green = {
    QColor(168, 226, 113),
    QColor(168, 226, 113),
    QColor("white"),
    5.0f
};

Material blue = {
    .ambient_color = QColor(113, 191, 226),
    .diffuse_color = QColor(113, 191, 226),
    .specular_color = QColor("white"),
    .shininess = 5.0f
};

Material pink = {
    .ambient_color = QColor(226, 113, 208),
    .diffuse_color = QColor(226, 113, 208),
    .specular_color = QColor("white"),
    .shininess = 5.0f
};

Material red = {
    .ambient_color = QColor(226, 113, 122),
    .diffuse_color = QColor(226, 113, 122),
    .specular_color = QColor("white"),
    .shininess = 5.0f
};

Material yellow = {
    .ambient_color = QColor(226, 223, 113),
    .diffuse_color = QColor(226, 223, 113),
    .specular_color = QColor("white"),
    .shininess = 5.0f
};

Material violet = {
    .ambient_color = QColor(136, 113, 226),
    .diffuse_color = QColor(136, 113, 226),
    .specular_color = QColor("white"),
    .shininess = 5.0f
};

std::vector<Material> predefined_colors = {
    green, blue, pink, red, yellow, violet
};



Material get_random_material() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 5);
    
    return predefined_colors[dis(gen)];
}