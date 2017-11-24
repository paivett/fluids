#include "openglfunctions.h"
#include "runtimeexception.h"

void OpenGLFunctions::init(QOpenGLFunctions_4_5_Core* gl) {
    _gl = gl;
    _gl->initializeOpenGLFunctions();
}

QOpenGLFunctions_4_5_Core& OpenGLFunctions::getFunctions() {
    if (_gl == nullptr) {
        throw RunTimeException("GL functions not initialized. Try calling init() before usage.");
    }
    return *_gl;
}

QOpenGLFunctions_4_5_Core* OpenGLFunctions::_gl = nullptr;