#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "graphicssettings.h"
#include "simulationsettings.h"
#include "physicssettings.h"

#include "runtimeexception.h"

#include <memory>

class Settings {
    public: 
        
        static void load(const std::string& config_file, 
                         const std::string& scene_file);

        static GraphicsSettings& graphics();

        static SimulationSettings& simulation();
 
        static PhysicsSettings& physics();

        static std::string scene_file();

        static void update(const GraphicsSettings& g_settings,
                           const SimulationSettings& s_settings,
                           const PhysicsSettings& p_settings);
        
        static void update_graphics(const GraphicsSettings& g_settings);
        
        static void update_simulation(const SimulationSettings& s_settings);
        
        static void update_physics(const PhysicsSettings& p_settings);

    private:
        Settings();
        Settings(const Settings&);
        Settings& operator=(const Settings&);

        static std::unique_ptr<GraphicsSettings> _graphics;
        static std::unique_ptr<SimulationSettings> _simulation;
        static std::unique_ptr<PhysicsSettings> _physics;
        static std::string _scene_file;

};

#endif // _SETTINGS_H_
