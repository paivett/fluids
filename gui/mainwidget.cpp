#include "mainwidget.h"

MainWidget::MainWidget(QWidget* parent) :
    QSplitter(parent) {

    _gl_widget = new GLWidget(this);
    _options_widget = new OptionsWidget(this);

    addWidget(_gl_widget);
    addWidget(_options_widget);

    connect(_options_widget,
            SIGNAL(settings_changed(SimulationSettings, PhysicsSettings, GraphicsSettings)),
            _gl_widget,
            SLOT(reset(SimulationSettings, PhysicsSettings, GraphicsSettings)));

    connect(_options_widget,
            SIGNAL(pause_simulation()),
            _gl_widget,
            SLOT(pause_simulation()));

    connect(_options_widget,
            SIGNAL(unpause_simulation()),
            _gl_widget,
            SLOT(unpause_simulation()));
}

GLWidget& MainWidget::get_gl_widget() {
	return *_gl_widget;
}

MainWidget::~MainWidget() {

}
