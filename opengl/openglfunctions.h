#ifndef _OPENGL_CONTEXT_H_
#define _OPENGL_CONTEXT_H_

#include <QOpenGLFunctions_4_5_Core>

class OpenGLFunctions {
    public:
        static void init(QOpenGLFunctions_4_5_Core* gl);

        static QOpenGLFunctions_4_5_Core& getFunctions();

    private:
        static QOpenGLFunctions_4_5_Core* _gl;
};

#endif // _OPENGL_CONTEXT_H_