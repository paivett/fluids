#ifndef _OPTIONS_WIDGET_H_
#define _OPTIONS_WIDGET_H_

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QDialogButtonBox>
#include "simulationoptionstab.h"
#include "physicsoptionstab.h"
#include "graphicsoptionstab.h"

class OptionsWidget : public QWidget {
    Q_OBJECT

    public:
        OptionsWidget(QWidget *parent = 0);
        ~OptionsWidget();

    signals:
        void settings_changed(SimulationSettings s_settings,
                              PhysicsSettings p_settings,
                              GraphicsSettings g_settings);

        void pause_simulation();
        void unpause_simulation();

    private slots:
        void apply_click();
        void reset_click();
        void settings_changed();
        void reset_simulation_click();
        void pause_simulation_click();

    private:
        QVBoxLayout* _layout;

        QLabel* _title;
        QTabWidget* _tabs;

        SimulationOptionsTab* _simulation_tab;
        PhysicsOptionsTab* _physics_tab;
        GraphicsOptionsTab* _graphics_tab;

        QDialogButtonBox* _button_box;
        QPushButton* _pause_button;

        SimulationSettings _simulation_settings;
        PhysicsSettings _physics_settings;
        GraphicsSettings _graphics_settings;
};

#endif // _OPTIONS_WIDGET_H_
