#ifndef _CL_COMPILER_H_
#define _CL_COMPILER_H_

#include <string>
#include <list>
#include <map>
#include <memory>
#include "clprogram.h"

/**
 * @class CLCompiler
 * @brief Compiler Represents an OpenCL program
 * 
 * @details This class manages the compilation of OpenCL programs.
 */
class CLCompiler {
    
    public:
        /**
         * @brief Constructs a new CLCompiler
         */
        CLCompiler();

        /**
         * @brief Adds new source code to the compilation process
         * 
         * @param src OpenCL compliant code
         */
        void add_source(const std::string& src);

        /**
         * @brief Adds a new source file to the compilation process
         * 
         * @param source_path The path to the OpenCL code
         */
        void add_file_source(const std::string& source_path);

        /**
         * @brief Adds a new include path
         * 
         * @param source_path The path to the OpenCL code
         */
        void add_include_path(const std::string& include_path);

        /**
         * @brief Adds a new build option to the compiler. This option
         * 	      will be included when compiling the program.
         * 
         * @param build_opt The compilation option to include
         */
        void add_build_option(const std::string& build_opt);

        /**
         * @brief Clears all the build options that were previously defined
         * 	      will be included when compiling the program.
         */
        void clear_options();

        /**
         * @brief Defines a new constant. This constant will be passed 
         *        down to the preprocessor when compiling 
         *        (using the -D option). This is equivalent to insert a 
         *        define directive in the code
         * 
         * @param name The name of the constant
         */
        void define_constant(const std::string& name);

        /**
         * @brief Defines a new constant. This constant will be passed 
         *        down to the preprocessor when compiling 
         *        (using the -D option). This is equivalent to insert a 
         *        define directive in the code
         * 
         * @param name The name of the constant
         * @param value The value of the constant. Consider that the compiler
         *              makes a literal transcription of the value. Be careful
         */
        void define_constant(const std::string& name, const std::string& value);

        /**
         * @brief Defines a new numeric constant. This constant will be passed 
         *        down to the preprocessor when compiling 
         *        (using the -D option). This is equivalent to insert a 
         *        define directive in the code
         * 
         * @param name The name of the constant
         * @param value The value of the constant. It will be converted to a string
         */
        void define_constant(const std::string& name, int value);

        /**
         * @brief Defines a new numeric constant. This constant will be passed 
         *        down to the preprocessor when compiling 
         *        (using the -D option). This is equivalent to insert a 
         *        define directive in the code
         * 
         * @param name The name of the constant
         * @param value The value of the constant. It will be converted to a string
         * @param precision The number of decimals to print
         */
        void define_constant(const std::string& name, float value, int precision=6);

        /**
         * @brief Builds the program with the defined sources and options.
		 * 
         * @return A pointer to a newly created program instance
         */
        std::unique_ptr<CLProgram> build();

    private:
        // The list of build options to use during the compilation
        std::list<std::string> _options;

        // A list of constant definitions
        std::map<std::string, std::string> _constants;

        // A list of sources to compile
        std::list<std::string> _source_paths;

        // A vector of source code
        std::vector<std::string> _source_code;

        // A list of include paths to use during compilation
        std::list<std::string> _include_paths;

        std::string _get_build_options() const;

        cl_program _create_program() const;
};

#endif // _CL_COMPILER_H_
