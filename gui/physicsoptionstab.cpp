#include "physicsoptionstab.h"
#include <QFormLayout>
#include <QIntValidator>

PhysicsOptionsTab::PhysicsOptionsTab(const PhysicsSettings& initial_settings,
                                     QWidget *parent) :
QWidget(parent) {
    _density_line = new QLineEdit(this);
    _density_line->setValidator(new QDoubleValidator(0, 5000, 4, this));
    connect(_density_line, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _k_viscosity_line = new QLineEdit(this);
    _k_viscosity_line->setValidator(new QDoubleValidator(0, 100, 10, this));
    connect(_k_viscosity_line, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _gravity_line = new QLineEdit(this);
    _gravity_line->setValidator(new QDoubleValidator(0, 100, 3, this));
    connect(_gravity_line, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _gas_stiffness_line = new QLineEdit(this);
    _gas_stiffness_line->setValidator(new QDoubleValidator(0, 100, 2, this));
    connect(_gas_stiffness_line, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _surface_tension_line = new QLineEdit(this);
    _surface_tension_line->setValidator(new QDoubleValidator(0, 10, 6, this));
    connect(_surface_tension_line, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    QFormLayout* formLayout = new QFormLayout;
    formLayout->addRow(tr("&Density:"), _density_line);
    formLayout->addRow(tr("&Kinetic viscosity:"), _k_viscosity_line);
    formLayout->addRow(tr("&Gravity:"), _gravity_line);
    formLayout->addRow(tr("&Gas stiffness:"), _gas_stiffness_line);
    formLayout->addRow(tr("&Surface tension:"), _surface_tension_line);

    setLayout(formLayout);

    set_settings(initial_settings);
}

PhysicsOptionsTab::~PhysicsOptionsTab() {

}

void PhysicsOptionsTab::set_settings(const PhysicsSettings& s) {
    _density_line->setText(QString::number(s.rest_density));
    _k_viscosity_line->setText(QString::number(s.k_viscosity));
    _gravity_line->setText(QString::number(s.gravity));
    _gas_stiffness_line->setText(QString::number(s.gas_stiffness));
    _surface_tension_line->setText(QString::number(s.surface_tension));
}

PhysicsSettings PhysicsOptionsTab::get_settings() const {
    PhysicsSettings s;
    s.rest_density = _density_line->text().toFloat();
    s.k_viscosity = _k_viscosity_line->text().toFloat();
    s.gravity = _gravity_line->text().toFloat();
    s.gas_stiffness = _gas_stiffness_line->text().toFloat();
    s.surface_tension = _surface_tension_line->text().toFloat();

    return s;
}

void PhysicsOptionsTab::_text_edited(const QString& text) {
    emit(settings_changed());
}
