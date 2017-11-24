#ifndef _GRAPHICS_OPTIONS_TAB_H_
#define _GRAPHICS_OPTIONS_TAB_H_

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>

#include <settings/settings.h>

class GraphicsOptionsTab : public QWidget {
    Q_OBJECT

    public:
        GraphicsOptionsTab(const GraphicsSettings& initial_settings,
                           QWidget *parent = 0);
        ~GraphicsOptionsTab();

        // Sets the values of the fields with the values of settings in s param
        void set_settings(const GraphicsSettings& s);

        // Returns the settings
        GraphicsSettings get_settings() const;

    signals:
        // Signals the fact that the settings have been manually modified
        void settings_changed();

    private:
        QPushButton* _fluid_color_button;
        QComboBox* _render_method;
        
        // Screen space fluid rendering controls
        QGroupBox* _sspace_group;
        QLineEdit* _sspace_filter_iterations;

        void _set_button_color(QPushButton* b, const QColor& c);
        QColor _get_button_color(QPushButton* b) const;
        void _display_sspace_options(bool show);

    private slots:
        void _pick_fluid_color();
        void _change_render_method(int idx);
        void _text_edited(const QString&);
};

#endif // _GRAPHICS_OPTIONS_TAB_H_
