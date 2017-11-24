#include "fileparser.h"
#include <fstream>
#include <sstream>

using namespace std;

FileParser::FileParser(const string& filename) {
    ifstream conf_file(filename);

    if(!conf_file.good()) {
        string msg = "Configuration file '";
        msg += filename + "' could not be parsed.";
        throw runtime_error(msg);
    }

    while(conf_file.good()) {
        string line;
        getline(conf_file, line);
        auto assign_pos = line.find_first_of("=");
        if (assign_pos != string::npos) {
            auto opt_name = _clean(line.substr(0, assign_pos));
            auto opt_val = _clean(line.substr(assign_pos+1));
            
            _options[opt_name] = opt_val;
        }
    }
}

const string& FileParser::option(const string& opt_name) const {
    if(has_option(opt_name)) {
        return _options.find(opt_name)->second;
    }
    else {
        string msg = "Option '";
        msg += opt_name + "' was not found in the configuration file";
        throw runtime_error(msg);
    }
}

bool FileParser::has_option(const string& opt_name) const {
    return _options.find(opt_name) != _options.end();
}

string FileParser::_clean(const string& s) const {
    stringstream ss;
    ss << s;
    string out;
    ss >> out;
    return out;
}