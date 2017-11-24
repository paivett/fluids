#ifndef _FILE_PARSER_H_
#define _FILE_PARSER_H_

#include <string>
#include <map>

class FileParser {
    public:
        FileParser(const std::string& filename);

        const std::string& option(const std::string& opt_name) const;

        bool has_option(const std::string& opt_name) const;

    private:
        std::map<std::string, std::string> _options;

        std::string _clean(const std::string& s) const;

};

#endif // _FILE_PARSER_H_