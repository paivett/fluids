#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <fstream>
#include <QMainWindow>
#include <QMenu>
#include <QLabel>
#include <QSharedPointer>

#include "mainwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        MainWindow(const std::string& fps_prof_output, QWidget *parent = 0);
        ~MainWindow();

    public slots:
        void showAboutDialog();
        void resetSimulation();

    private slots:
        void update_fps(float fps);
        void save_fps(float fps);
        void update_particle_count(int fluid_count, int boudnary_count);

    private:
        void setUpMenuBar();
        void setUpStatusBar();

        // GUI Elements
        QLabel* _fps_label;
        QLabel* _fluid_particles_count;
        QLabel* _boundary_particles_count;

        std::ofstream _fps_prof_fp;

        MainWidget* _main_widget;
};

#endif // _MAINWINDOW_H_
