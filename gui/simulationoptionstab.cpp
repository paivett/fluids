#include "simulationoptionstab.h"
#include "fluid/simulation/fluidsimulation.h"
#include <QFormLayout>
#include <QIntValidator>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSizePolicy>

SimulationOptionsTab::SimulationOptionsTab(const SimulationSettings& initial_settings,
                                           QWidget *parent) :
QWidget(parent) {
    QVBoxLayout* main_layout = new QVBoxLayout();

    QWidget* main_top_widget = new QWidget();
    QFormLayout* main_top_layout = new QFormLayout();
    
    _time_step = new QLineEdit(this);
    _time_step->setValidator(new QDoubleValidator(0, 10, 6, this));
    connect(_time_step, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _max_vel = new QLineEdit(this);
    _max_vel->setValidator(new QDoubleValidator(0, 100, 6, this));
    connect(_max_vel, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _pcisph_max_iterations = new QLineEdit(this);
    _pcisph_max_iterations->setValidator(new QIntValidator(3, 20, this));
    connect(_pcisph_max_iterations, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _pcisph_error_ratio = new QLineEdit(this);
    _pcisph_error_ratio->setValidator(new QDoubleValidator(0, 1, 6, this));
    connect(_pcisph_error_ratio, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _simulation_method = new QComboBox();
    _simulation_method->addItem("WCSPH", SimulationSettings::Method::WCSPH);
    _simulation_method->addItem("PCISPH", SimulationSettings::Method::PCISPH);
    connect(_simulation_method, SIGNAL(currentIndexChanged(int)),
            this, SLOT(_method_changed(int)));
    _simulation_method->setEditable(false);
    _simulation_method->setEnabled(false);


    main_top_layout->addRow(tr("&Method:"), _simulation_method);
    main_top_layout->addRow(tr("&Time delta:"), _time_step);
    main_top_layout->addRow(tr("&Max particle vel:"), _max_vel);
    main_top_layout->addRow(tr("&(PCISPH) Max iter:"), _pcisph_max_iterations);
    main_top_layout->addRow(tr("&(PCISPH) Error ratio:"), _pcisph_error_ratio);
    main_top_widget->setLayout(main_top_layout);
    main_top_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


    QGroupBox* fluid_group = new QGroupBox("Fluid");

    QFormLayout* fluid_form_layout = new QFormLayout();

    // Fluid options
    _fluid_particle_radius = new QLineEdit();
    _fluid_particle_radius->setValidator(new QDoubleValidator(0.000001, 10, 6, this));
    connect(_fluid_particle_radius, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));
    
    _fluid_support_radius = new QLineEdit();
    _fluid_support_radius->setValidator(new QDoubleValidator(0.000001, 10, 6, this));
    connect(_fluid_support_radius, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    fluid_form_layout->addRow(tr("&Particle radius:"), _fluid_particle_radius);
    fluid_form_layout->addRow(tr("&Support radius:"), _fluid_support_radius);

    fluid_group->setLayout(fluid_form_layout);
    fluid_group->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    main_layout->addWidget(main_top_widget);
    main_layout->addWidget(fluid_group);
    //main_layout->addWidget(boundary_group);
    // Add a padding widget
    main_layout->addWidget(new QWidget());
    setLayout(main_layout);
    
    set_settings(initial_settings);
}

SimulationOptionsTab::~SimulationOptionsTab() {

}

void SimulationOptionsTab::set_settings(const SimulationSettings& s) {
    _fluid_particle_radius->setText(QString::number(s.fluid_particle_radius));
    _fluid_support_radius->setText(QString::number(s.fluid_support_radius));
    _time_step->setText(QString::number(s.time_step));
    _max_vel->setText(QString::number(s.max_vel));
    _pcisph_max_iterations->setText(QString::number(s.pcisph_max_iterations));
    _pcisph_error_ratio->setText(QString::number(s.pcisph_error_ratio));

    int index = _simulation_method->findData(s.sim_method);
    _simulation_method->setCurrentIndex(index);
}

SimulationSettings SimulationOptionsTab::get_settings() const {
    SimulationSettings s;
    s.time_step = _time_step->text().toFloat();
    s.max_vel = _max_vel->text().toFloat();
    s.fluid_particle_radius = _fluid_particle_radius->text().toFloat();
    s.fluid_support_radius = _fluid_support_radius->text().toFloat();
    s.pcisph_max_iterations = _pcisph_max_iterations->text().toInt();
    s.pcisph_error_ratio = _pcisph_error_ratio->text().toFloat();
    s.sim_method = (SimulationSettings::Method)_simulation_method->currentData().toInt();

    return s;
}

void SimulationOptionsTab::_text_edited(const QString& text) {
    emit(settings_changed());
}

void SimulationOptionsTab::_method_changed(int method_index) {
    emit(settings_changed());   
}