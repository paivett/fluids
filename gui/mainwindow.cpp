#include "mainwindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QLabel>
#include <QFrame>

using namespace std;

MainWindow::MainWindow(const string& fps_prof_output, QWidget *parent)
    : QMainWindow(parent)
{
    //Configure window
    setWindowTitle("Smoothed Particle Hydrodynamics");
    setFixedSize(1024, 768);

    setUpMenuBar();
    setUpStatusBar();

    _main_widget = new MainWidget(this);
    setCentralWidget(_main_widget);

    connect(&_main_widget->get_gl_widget(),
            SIGNAL(new_frame(float)),
            this,
            SLOT(update_fps(float)));

    connect(&_main_widget->get_gl_widget(),
            SIGNAL(particle_count_changed(int, int)),
            this,
            SLOT(update_particle_count(int, int)));

    if (fps_prof_output != "") {
        _fps_prof_fp.open(fps_prof_output.c_str());
        if (_fps_prof_fp.good()) {
            connect(&_main_widget->get_gl_widget(),
                    SIGNAL(new_frame(float)),
                    this,
                    SLOT(save_fps(float)));
        }
    }
}

MainWindow::~MainWindow() {
    if (_fps_prof_fp.is_open()) {
        _fps_prof_fp.close();
    }
}

void MainWindow::setUpMenuBar() {
    //File menu
    QMenu* menu = menuBar()->addMenu(tr("&File"));

    QAction *action = menu->addAction(tr("Quit"));
    connect(action, SIGNAL(triggered()), this, SLOT(close()));

    //Help menu
    menu = menuBar()->addMenu(tr("&Help"));

    action = menu->addAction(tr("About"));
    connect(action, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
}

void MainWindow::setUpStatusBar() {
    QStatusBar* sBar = statusBar();
    sBar->setSizeGripEnabled(false);

    _fps_label = new QLabel("FPS: 0");
    sBar->addPermanentWidget(_fps_label, 1);

    _fluid_particles_count = new QLabel("Fluid particles: 0");
    sBar->addPermanentWidget(_fluid_particles_count, 0);

    _boundary_particles_count = new QLabel("Boundary particles: 0");
    sBar->addPermanentWidget(_boundary_particles_count, 0);
}

void MainWindow::showAboutDialog() {
    QMessageBox::about(this,
                       "About Smoothed Particle Hydrodynamics",
                       "<b>Fluid-Rigid body interaction with SPH</b><br>\
                       2017 Master thesis, FCEyN<br>Santiago Daniel Pivetta<br>\
                       version 0.8");
}

void MainWindow::resetSimulation() {

}

void MainWindow::update_particle_count(int fluid_count, int boundary_count) {
    _fluid_particles_count->setText(QString("Fluid particles: ") + QString::number(fluid_count));
    _boundary_particles_count->setText(QString("Boundary particles: ") + QString::number(boundary_count));
}

void MainWindow::update_fps(float fps) {
    _fps_label->setText(QString("FPS: ") + QString::number(fps));
}

void MainWindow::save_fps(float fps) {
    _fps_prof_fp << fps << "\n";    
}
