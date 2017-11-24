#include "optionswidget.h"
#include <QPushButton>
#include <iostream>

OptionsWidget::OptionsWidget(QWidget* parent) :
QWidget(parent),
_simulation_settings(Settings::simulation()),
_physics_settings(Settings::physics()),
_graphics_settings(Settings::graphics()) {
    _layout = new QVBoxLayout(this);
    _layout->setAlignment(Qt::AlignTop);

    _title = new QLabel("Configuration", this);

    _simulation_tab = new SimulationOptionsTab(_simulation_settings, this);
    _physics_tab = new PhysicsOptionsTab(_physics_settings, this);
    _graphics_tab = new GraphicsOptionsTab(_graphics_settings, this);

    _tabs = new QTabWidget(this);
    _tabs->addTab(_simulation_tab, tr("Simulation"));
    _tabs->addTab(_physics_tab, tr("Physics"));
    _tabs->addTab(_graphics_tab, tr("Graphics"));

    _button_box = new QDialogButtonBox(this);
    _button_box->addButton("Apply", QDialogButtonBox::YesRole);
    _button_box->addButton("Cancel", QDialogButtonBox::NoRole);

    connect(_button_box, SIGNAL(accepted()), this, SLOT(apply_click()));
    connect(_button_box, SIGNAL(rejected()), this, SLOT(reset_click()));

    connect(_simulation_tab, SIGNAL(settings_changed()), this, SLOT(settings_changed()));
    connect(_physics_tab, SIGNAL(settings_changed()), this, SLOT(settings_changed()));
    connect(_graphics_tab, SIGNAL(settings_changed()), this, SLOT(settings_changed()));

    _button_box->setCenterButtons(true);
    _button_box->setEnabled(false);

    _layout->addWidget(_title);
    _layout->addWidget(_tabs);
    _layout->addWidget(_button_box);

    // Reset simulation button
    QPushButton* reset_button = new QPushButton("Reset", this);
    connect(reset_button, SIGNAL(clicked()), this, SLOT(reset_simulation_click()));
    _layout->addWidget(reset_button);

    // Pause simulation button
    _pause_button = new QPushButton("Pause", this);
    connect(_pause_button, SIGNAL(clicked()), this, SLOT(pause_simulation_click()));
    _layout->addWidget(_pause_button);

    setFixedWidth(250);
}

OptionsWidget::~OptionsWidget() {

}

void OptionsWidget::apply_click() {
    _button_box->setEnabled(false);
    // Update internal copy of settings
    _simulation_settings = _simulation_tab->get_settings();
    _physics_settings = _physics_tab->get_settings();
    _graphics_settings = _graphics_tab->get_settings();
    // emit signal!
    emit(settings_changed(_simulation_settings, _physics_settings, _graphics_settings));

    // Also, update the global settings
    Settings::update(_graphics_settings, _simulation_settings, _physics_settings);
}

void OptionsWidget::reset_click() {
    _button_box->setEnabled(false);
    // Update internal copy of settings
    _simulation_tab->set_settings(_simulation_settings);
    _physics_tab->set_settings(_physics_settings);
    _graphics_tab->set_settings(_graphics_settings);
}

void OptionsWidget::settings_changed() {
    _button_box->setEnabled(true);
}

void OptionsWidget::reset_simulation_click() {
    // Undo any change on the settings
    reset_click();
    // emit a fake settings changed to restart simulation
    apply_click();
}

void OptionsWidget::pause_simulation_click() {
    if (_pause_button->text() == "Pause") {
        emit(pause_simulation());
        _pause_button->setText("Unpause");
    }
    else {
        emit(unpause_simulation());
        _pause_button->setText("Pause");
    }

}
