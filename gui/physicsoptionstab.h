#ifndef _PHYSICS_OPTIONSTAB_H_
#define _PHYSICS_OPTIONSTAB_H_

#include <QWidget>
#include <QLineEdit>

#include <settings/settings.h>

class PhysicsOptionsTab : public QWidget {
    Q_OBJECT

    public:
        PhysicsOptionsTab(const PhysicsSettings& initial_settings,
                          QWidget *parent = 0);

        ~PhysicsOptionsTab();

        // Sets the values of the fields with the values of settings in s param
        void set_settings(const PhysicsSettings& s);

        // Returns the settings
        PhysicsSettings get_settings() const;

    signals:
        // Signals the fact that the settings have been manually modified
        void settings_changed();

    private slots:
        void _text_edited(const QString& text);

    private:
        QLineEdit* _density_line;
        QLineEdit* _k_viscosity_line;
        QLineEdit* _gravity_line;
        QLineEdit* _gas_stiffness_line;
        QLineEdit* _surface_tension_line;
};

#endif // _PHYSICS_OPTIONSTAB_H_
