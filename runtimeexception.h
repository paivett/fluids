#ifndef _RUN_TIME_EXCEPTION_H_
#define _RUN_TIME_EXCEPTION_H_

#include <QException>
#include <QString>
#include <string>

class RunTimeException : public QException {
    public:
        RunTimeException(const std::string& what): _what(what) { }

        //RunTimeException(const QString& what): _what(what.toStdString()) { }

        ~RunTimeException() throw() {}

        void raise() const {
            throw *this;
        }

        RunTimeException* clone() const {
            return new RunTimeException(*this);
        }

        const char* what() const throw() {
            return _what.c_str();
        }

    protected:
        std::string _what;
};

#endif // _RUN_TIME_EXCEPTION_H_