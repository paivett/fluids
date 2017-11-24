#ifndef _GLWIDGET_H_
#define _GLWIDGET_H_

#include <memory>

#include <QOpenGLWidget>
#include <QSharedPointer>

#include "scene/scene.h"
#include "settings/settings.h"

class GLWidget : public QOpenGLWidget {
    Q_OBJECT

    public:
        GLWidget(QWidget *parent = 0);
        ~GLWidget();

        QSize minimumSizeHint() const;
        QSize sizeHint() const;

    public slots:
        void reset(SimulationSettings s_settings,
                   PhysicsSettings p_settings,
                   GraphicsSettings g_settings);
        void pause_simulation();
        void unpause_simulation();

    protected:
        void initializeGL();
        void paintGL();
        void resizeGL(int width, int height);
        void mousePressEvent(QMouseEvent *event);
        void mouseMoveEvent(QMouseEvent *event);
        void wheelEvent(QWheelEvent * event);
        void init_scene();

    signals:
        void new_frame(float current_fps);
        void particle_count_changed(int fluid_count, int boundary_count);

    private:
        //The scene holds all renderable objects as well
        //as the camera with its projection
        std::unique_ptr<Scene> scene;
        bool _paused;
        QVector2D mousePressPosition;
};

#endif