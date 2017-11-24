#ifndef _MAIN_WIDGET_H_
#define _MAIN_WIDGET_H_

#include <QSplitter>
#include <QHBoxLayout>
#include "glwidget.h"
#include "optionswidget.h"

class MainWidget : public QSplitter {
    Q_OBJECT

    public:
        MainWidget(QWidget *parent = 0);
        ~MainWidget();

        // Returns a reference to the current GL widget where simulation
        // is being rendered
        GLWidget& get_gl_widget();

        // Returns a reference to the options widget
        OptionsWidget& get_options_widget();

    private:
        GLWidget* _gl_widget;
        OptionsWidget* _options_widget;
};

#endif // _MAIN_WIDGET_H_
