#include "clcompiler.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

CLCompiler::CLCompiler() {

}

void CLCompiler::add_source(const std::string& src) {
    _source_code.push_back(src);
}


void CLCompiler::add_file_source(const std::string& source_path) {
    _source_paths.push_back(source_path);
}


void CLCompiler::add_include_path(const std::string& include_path) {
    _include_paths.push_back(include_path);
}


void CLCompiler::add_build_option(const string& build_opt) {
    _options.push_back(build_opt);
}


void CLCompiler::clear_options() {
    _options.clear();
}

void CLCompiler::define_constant(const string& name) {
    _constants[name] = "1";
}

void CLCompiler::define_constant(const string& name, const string& value) {
    _constants[name] = value;
}

void CLCompiler::define_constant(const string& name, int value) {
    _constants[name] = to_string(value);
}

void CLCompiler::define_constant(const string& name, float value, int precision) {
    ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    _constants[name] = out.str() + "f";
}

unique_ptr<CLProgram> CLCompiler::build() {
    cl_int err;
    cout << "Commencing OpenCL program compilation process..." << endl;

    auto program = _create_program();
    
    cout << "Building program..." << endl;

    auto options = _get_build_options();

    cout << "Using options: " << options << endl;

    auto dev_id = CLEnvironment::device();
    err = clBuildProgram(program,
                         1,
                         &dev_id,
                         options.c_str(),
                         NULL,
                         NULL);
    
    if (err != CL_SUCCESS) {
        // Determine the size of the log
        size_t log_size;
        clGetProgramBuildInfo(program,
                              dev_id, 
                              CL_PROGRAM_BUILD_LOG, 
                              0, 
                              NULL, 
                              &log_size);

        // Allocate memory for the log
        char *log = (char *) malloc(log_size);

        // Get the log
        clGetProgramBuildInfo(program, 
                              dev_id, 
                              CL_PROGRAM_BUILD_LOG, 
                              log_size, 
                              log, 
                              NULL);

        // Print the log
        cout << log << endl;

        free(log);
    }

    CLError::check(err);

    cout << "Build complete!" << endl;

    return unique_ptr<CLProgram>(new CLProgram(program));
}

cl_program CLCompiler::_create_program() const {
    cl_int err;

    vector<string> sources(_source_code);

    // Now load code defined from files
    for(auto path : _source_paths) {
        cout << "Loading source: " << path << endl;

        ifstream f(path.c_str());
        if (!f.good()) {
            string msg = "Could not open program source: ";
            throw runtime_error(msg + path);
        }

        string src((istreambuf_iterator<char>(f)),
                    istreambuf_iterator<char>());

        // Add an extra new line to prevent
        // problems when compiling, since the function 
        // clCreateProgramWithSource seems to concatenate 
        // all sources

        src += '\n';

        sources.push_back(src);
    }

    vector<const char*> ptr_sources;
    vector<size_t> sizes;
    for(auto& src : sources) {
        ptr_sources.push_back(src.c_str());
        sizes.push_back(src.size());
    }

    auto program = clCreateProgramWithSource(CLEnvironment::context(),
                                             ptr_sources.size(),
                                             ptr_sources.data(),
                                             sizes.data(),
                                             &err);

    CLError::check(err);

    return program;
}

std::string CLCompiler::_get_build_options() const {
    string options;
    for (auto& opt : _options) {
        options += opt + " ";
    }

    for (auto& path : _include_paths) {
        options += "-I" + path + " ";
    }

    for (auto& kv : _constants) {
        options += "-D " + kv.first + "=" + kv.second + " ";
    }

    return options;
}