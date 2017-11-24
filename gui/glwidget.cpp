#include <QtWidgets>
#include <QtOpenGL>
#include <QSurfaceFormat>
#include <QOpenGLVersionProfile>

#include <math.h>
#include <chrono>

#include "glwidget.h"
#include "opengl/openglfunctions.h"
#include "scene/fluid.h"
#include "opencl/clenvironment.h"
#include <QOpenGLFunctions_4_5_Core>

using namespace std;

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      _paused(false) {

    setMouseTracking(false);
}

GLWidget::~GLWidget() {

}

QSize GLWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const {
    return QSize(400, 400);
}

void GLWidget::initializeGL() {
    // Here we have an opengl context initialized, so initialize OpenGLContext class
    auto gl_functions = context()->versionFunctions<QOpenGLFunctions_4_5_Core>();
    OpenGLFunctions::init(gl_functions);

    // Now that opengl has been initializated, it is safe to init cl enviroment
    CLEnvironment::init();

    glEnable(GL_DEPTH_TEST);

    init_scene();
}

void GLWidget::paintGL() {
    auto t0 = chrono::high_resolution_clock::now();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float time_step = Settings::simulation().time_step;
    if (_paused) {
        time_step = 0.0f;
    }
    scene->render(time_step, defaultFramebufferObject());

    auto tf = chrono::high_resolution_clock::now();
    auto d = chrono::duration_cast<chrono::milliseconds>(tf-t0);
    
    static int frame_count = 0;
    static float total_ms = 0;

    ++frame_count;
    total_ms += d.count();

    if (frame_count > 5) {
        float fps = 1000.0 / (total_ms / frame_count);
        total_ms = 0;
        frame_count = 0;
        emit(new_frame(fps));
    }

    update();
}

void GLWidget::resizeGL(int w, int h) {
    //Set OpenGL viewport to cover whole widget
    glViewport(0, 0, w, h);

    Perspective p;
    p.aspect = (float)w / (float)(h ? h : 1);
    p.nearPlane = 0.5;
    p.farPlane = 30.0;
    p.fov = 45.0;
    p.width = w;
    p.height = h;
    scene->camera().set_perspective(p);
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        mousePressPosition = QVector2D(event->pos());
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::RightButton) {
        QVector2D currentPos = QVector2D(event->pos());
        QVector2D v = mousePressPosition - currentPos;
        float pitch = v.x();
        float roll = v.y();

        scene->camera().add_roll(roll * -0.5);
        scene->camera().add_pitch(pitch * -0.5);

        mousePressPosition = currentPos;
    }
}

void GLWidget::wheelEvent(QWheelEvent * event) {
    scene->camera().add_zoom(event->delta() * -0.005);
}

void GLWidget::init_scene() {
    int w = width();
    int h = height();

    scene = Scene::load_scene(Settings::scene_file(), w, h);

    //Set up camera position
    scene->camera().look_at(QVector3D(0,3,3),
                            QVector3D(0,0,0),
                            QVector3D(0,1,0));

    auto fluid = std::dynamic_pointer_cast<Fluid>(scene->get_object("fluid"));
    emit(particle_count_changed(fluid->particle_count(), fluid->boundary_particle_count()));
}

void GLWidget::reset(SimulationSettings s_settings,
                     PhysicsSettings p_settings,
                     GraphicsSettings g_settings) {
    makeCurrent();

    scene->reset(s_settings, p_settings, g_settings);
    auto fluid = std::dynamic_pointer_cast<Fluid>(scene->get_object("fluid"));
    if (fluid) {
        emit(particle_count_changed(fluid->particle_count(), fluid->boundary_particle_count()));
    }

    doneCurrent();
}

void GLWidget::pause_simulation() {
    _paused = true;
}

void GLWidget::unpause_simulation() {
    _paused = false;
}