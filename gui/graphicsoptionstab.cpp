#include "graphicsoptionstab.h"
#include <QFormLayout>
#include <QIntValidator>
#include <QColorDialog>

#include <iostream>

GraphicsOptionsTab::GraphicsOptionsTab(const GraphicsSettings& initial_settings,
                                       QWidget *parent) :
QWidget(parent) {
    _fluid_color_button = new QPushButton(this);
    _fluid_color_button->setMaximumWidth(50);
    connect(_fluid_color_button,
            SIGNAL(released()),
            this,
            SLOT(_pick_fluid_color()));

    _render_method = new QComboBox(this);
    _render_method->addItem("Particles", PARTICLES);
    _render_method->addItem("Screen Space Fluid Rendering", SCREEN_SPACE);
    connect(_render_method,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(_change_render_method(int)));

    _sspace_filter_iterations = new QLineEdit(this);
    _sspace_filter_iterations->setValidator(new QIntValidator(1, 100, this));
    connect(_sspace_filter_iterations, SIGNAL(textEdited(const QString&)),
            this, SLOT(_text_edited(const QString&)));

    _sspace_group = new QGroupBox("Screen space method properties", this);
    QFormLayout* sspace_group_layout = new QFormLayout();
    sspace_group_layout->addRow(tr("&Filter iterations:"), _sspace_filter_iterations);
    _sspace_group->setLayout(sspace_group_layout);

    QFormLayout* form_layout = new QFormLayout();
    form_layout->addRow(tr("&Fluid color:"), _fluid_color_button);
    form_layout->addRow(tr("&Method:"), _render_method);
    form_layout->addRow(_sspace_group);
    
    setLayout(form_layout);

    set_settings(initial_settings);
}

GraphicsOptionsTab::~GraphicsOptionsTab() {
    delete _fluid_color_button;
    delete _render_method;
}

void GraphicsOptionsTab::set_settings(const GraphicsSettings& s) {
    _set_button_color(_fluid_color_button, s.fluid_color);
    _render_method->setCurrentIndex(s.render_method);
    _sspace_filter_iterations->setText(QString::number(s.sspace_filter_iterations));
}

GraphicsSettings GraphicsOptionsTab::get_settings() const {
    GraphicsSettings s;

    s.fluid_color = _get_button_color(_fluid_color_button);
    s.render_method = (RenderMethod)_render_method->itemData(_render_method->currentIndex()).value<int>();
    s.sspace_filter_iterations = _sspace_filter_iterations->text().toInt();

    return s;
}

void GraphicsOptionsTab::_set_button_color(QPushButton* b, const QColor& c) {
    QPalette p;
    p.setColor(QPalette::Button , c);
    b->setPalette(p);
    b->setAutoFillBackground(true);
    b->setFlat(true);
}

void GraphicsOptionsTab::_pick_fluid_color() {
    QColor current = _get_button_color(_fluid_color_button);
    QColor new_color = QColorDialog::getColor(current, this, "Fluid color");
    _set_button_color(_fluid_color_button, new_color);
    emit(settings_changed());
}

QColor GraphicsOptionsTab::_get_button_color(QPushButton* b) const {
    return b->palette().color(QPalette::Button);
}

void GraphicsOptionsTab::_change_render_method(int idx) {
    auto new_method = (RenderMethod)_render_method->itemData(idx).value<int>();
    _display_sspace_options(new_method == SCREEN_SPACE);
    emit(settings_changed());
}

void GraphicsOptionsTab::_display_sspace_options(bool show) {
    if (show) {
        _sspace_group->show();
    }
    else {
        _sspace_group->hide();
    }
}

void GraphicsOptionsTab::_text_edited(const QString&) {
    emit(settings_changed());
}