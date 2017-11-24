#include "gui/mainwindow.h"
#include "settings/settings.h"
#include <QDir>
#include <QApplication>
#include <QStyleFactory>
#include <cstdlib>
#include <iostream>
#include <cxxopts/cxxopts.hpp>

using namespace std;

int main(int argc, char *argv[]) {
    cxxopts::Options options("sph", "SPH fluid simulation");
            
    options.add_options()
        ("help", "Print help")
        ("s,scene", "Scene file path", cxxopts::value<std::string>())
        ("c,config", "Config file path", cxxopts::value<std::string>())
        ("o,perfomance_output", "Fps performance output file path", cxxopts::value<std::string>());
    
    try {
        options.parse(argc, argv);

        if (options.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        cxxopts::check_required(options, {"scene", "config"});

        auto config_filename = options["config"].as<std::string>();
        auto scene_filename = options["scene"].as<std::string>();
        string profiling_filename = "";
        if (options.count("profile_output")) {
            profiling_filename = options["profile_output"].as<std::string>();
        }

        // Create directory for kernels profile
        if (!QDir("k_profile").exists()) {
            QDir().mkdir("k_profile");
        }

        // Disable CUDA/OpenCL compiler caching(NVIDIA-only)
        setenv("CUDA_CACHE_DISABLE", "1", 1);

        // Before initializing Qt Application, settings must be loaded
        Settings::load(config_filename, scene_filename);

        // Configure the application to use OpenGL 4.5
        QSurfaceFormat format;
        format.setVersion(4, 5);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setSwapInterval(0); // This turns off vsync
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        QSurfaceFormat::setDefaultFormat(format);

        QApplication a(argc, argv);

        a.setStyle(QStyleFactory::create("Fusion"));
         
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53,53,53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25,25,25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53,53,53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
         
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);

        a.setPalette(darkPalette);
         
        a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

        MainWindow w(profiling_filename);
        w.show();

        return a.exec();
    }
    catch(cxxopts::OptionException& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        std::exit(-1);
    }
    catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(-1);
    }
}
