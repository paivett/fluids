#ifndef _SIMULATION_OPTIONS_TAB_H_
#define _SIMULATION_OPTIONS_TAB_H_

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>

#include <settings/settings.h>

class SimulationOptionsTab : public QWidget {
    Q_OBJECT

    public:
        SimulationOptionsTab(const SimulationSettings& initial_settings,
                             QWidget *parent = 0);
        ~SimulationOptionsTab();

        // Sets the values of the fields with the values of settings in s param
        void set_settings(const SimulationSettings& s);

        // Returns the current settings
        SimulationSettings get_settings() const;

    signals:
        // Signals the fact that the settings have been manually modified
        void settings_changed();

    private slots:
        void _text_edited(const QString& text);
        void _method_changed(int method_index);

    private:
        QLineEdit* _time_step;
        QComboBox* _simulation_method;
        QLineEdit* _max_vel;

        // Fluid options
        QLineEdit* _fluid_particle_radius;
        QLineEdit* _fluid_support_radius;

        // Boundary options
        QLineEdit* _boundary_particle_radius;
        QLineEdit* _boundary_support_radius;
};

#endif // _SIMULATION_OPTIONS_TAB_H_
