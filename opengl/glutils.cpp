#include "glutils.h"
#include "runtimeexception.h"

using namespace std;

unique_ptr<QOpenGLShaderProgram> create_shader_program(const string& vertex_shader_filename,
                                                       const string& fragment_shader_filename) {
    auto shader = unique_ptr<QOpenGLShaderProgram>(new QOpenGLShaderProgram());

    if(!shader->addShaderFromSourceFile(QOpenGLShader::Vertex, vertex_shader_filename.c_str())) {
        throw RunTimeException(string("Could not load vertex shader") + vertex_shader_filename);
    }

    if (!shader->addShaderFromSourceFile(QOpenGLShader::Fragment, fragment_shader_filename.c_str())) {
        throw RunTimeException(string("Could not load fragment shader") + fragment_shader_filename);
    }

    if (!shader->link()) {
        throw RunTimeException("Could not link shader program");
    }

    return shader;
}
