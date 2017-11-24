#ifndef _OGL_UTILS_H_
#define _OGL_UTILS_H_

#include <memory>
#include <string>
#include <QOpenGLShaderProgram>

std::unique_ptr<QOpenGLShaderProgram> create_shader_program(const std::string& vertex_shader_filename,
                                                            const std::string& fragment_shader_filename);

#endif // _OGL_UTILS_H_