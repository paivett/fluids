#include <cmath>
#include <map>
#include <fstream>
#include "clkernel.h"
#include "clmisc.h"
#include "external/json/json11.hpp"

#include <iostream>

using namespace std;


CLKernel::CLKernel(cl_kernel k) : _kernel(k) {
    // Let's find out the kernel function name
    char k_name[256];
    auto err = clGetKernelInfo (_kernel,
                                CL_KERNEL_FUNCTION_NAME,
                                256, // this should be more than enough
                                k_name,
                                nullptr);

    CLError::check(err);
    
    _kernel_name = string(k_name);

    // Try to load a profile data for this kernel
    _prof_filename = "k_profile/" + _kernel_name;
    ifstream f(_prof_filename.c_str(), ifstream::in);
    if (f.good()) {
        string f_str((istreambuf_iterator<char>(f)),
                     istreambuf_iterator<char>());

        string parse_errors;
        auto profile_data = json11::Json::parse(f_str, parse_errors);
        
        if (parse_errors != "") {
            cerr << "Error parsing " << _prof_filename << ": " << parse_errors << endl;
        }

        for (auto& data : profile_data.object_items()) {
            auto& opt_data = _opt_data[std::stoi(data.first)];
            opt_data.rounds = _KERNEL_OPT_ROUNDS;
            
            auto size_data = data.second.object_items();

            size_t local_x = size_data["x"].int_value();
            size_t local_y = size_data["y"].int_value();
            size_t local_z = size_data["z"].int_value();

            opt_data.local_size = CLRange(local_x, local_y, local_z);
        }
    }
}

CLKernel::~CLKernel() {
    ofstream f(_prof_filename);

    map<string, json11::Json> out_data;

    for(auto &data : _opt_data) {
        if (data.second.rounds == _KERNEL_OPT_ROUNDS) {
            map<string, json11::Json> size;
            size["x"] = json11::Json((int)data.second.local_size.x());
            size["y"] = json11::Json((int)data.second.local_size.y());
            size["z"] = json11::Json((int)data.second.local_size.z());
            out_data[std::to_string(data.first)] = json11::Json(size);
        }
    }

    string dump_string;
    auto json_data = json11::Json(out_data);
    json_data.dump(dump_string);
    f << dump_string;
    f.close();
}

std::string CLKernel::name() const {
    return _kernel_name;
}

void CLKernel::set_local_buffer(cl_uint index, size_t size) {
    _local_buffers_info.push_back(pair<cl_uint, size_t>(index, size));
}

cl_int CLKernel::run(size_t work_item_count, size_t local_size) {
    return run(CLRange(work_item_count), CLRange(local_size), CLRange(32));
}

cl_int CLKernel::run(const CLRange& work_item_count, const CLRange& local_size, const CLRange& opt_step_size) {
    static cl_event event;
    auto& opt_data = _opt_data[work_item_count.count()];
    
    if (local_size.count() > 0) {
        // If we are given a size, then we do no optimization steps
        opt_data.local_size = local_size;
        opt_data.rounds = _KERNEL_OPT_ROUNDS;
    }
    else if (opt_data.local_size.count() == 0) {
        opt_data.local_size = opt_step_size;
    }

    if (opt_data.global_size.count() == 0) {
        opt_data.global_size = work_item_count + opt_data.local_size - (work_item_count % opt_data.local_size);
    }

    for (auto& info : _local_buffers_info) {
        clSetKernelArg(_kernel, info.first, info.second * opt_data.local_size.count(), nullptr);
    }

    cl_int err = clEnqueueNDRangeKernel(CLEnvironment::queue(),
                                        _kernel,
                                        opt_data.local_size.dims(),
                                        nullptr,
                                        opt_data.global_size.data(),
                                        opt_data.local_size.data(),
                                        0,
                                        nullptr,
                                        &event);
    
    if (opt_data.rounds < _KERNEL_OPT_ROUNDS) {
        if (opt_data.rounds == _KERNEL_OPT_ROUNDS - 1) {
            // Let's compute the average and keep the best
            CLRange opt_local_size;
            cl_ulong opt_time = 0;
            for(auto &time_data : opt_data.accum_time) {
                auto avg = time_data.second / _KERNEL_OPT_ROUNDS;
                if ((avg < opt_time) || (opt_time == 0)) {
                    opt_time = avg;
                    opt_local_size = opt_data.range[time_data.first];
                }
            }
            
            opt_data.local_size = opt_local_size;
            opt_data.global_size = work_item_count + opt_data.local_size - (work_item_count % opt_data.local_size);
        }
        else {
            clWaitForEvents(1 , &event);
            opt_data.accum_time[opt_data.local_size.count()] += execution_time(event) / 1000;
            opt_data.range[opt_data.local_size.count()] = opt_data.local_size;
            
            opt_data.local_size = opt_data.local_size + opt_step_size;
            if (opt_data.local_size.count() > 1024) {
                opt_data.local_size = opt_step_size;
            }
            opt_data.global_size = work_item_count + opt_data.local_size - (work_item_count % opt_data.local_size);
        }
        
        ++(opt_data.rounds);
    }

    return err;
}