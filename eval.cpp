#include "Variable.h"
#include "Circuit.h"
#include "SCDLProgram.h"
#include "SCDLEvaluator.h"
#include <fstream>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


using namespace scdl;



void run(const std::string &scdl_file)
{
    std::ifstream scdl_in(scdl_file.c_str());
    if (!scdl_in.good()) {
        std::cerr << scdl_file << " not found" << std::endl;
        return;
    }
    
    std::ifstream vars_in((scdl_file + ".vars").c_str());
    if (!vars_in.good()) {
        vars_in.close();
        std::cerr << "No .vars file found" << std::endl;
        return;
    }

    CompilerResult result = SCDLEvaluator::compile(scdl_in, vars_in);
    SCDLEvaluator::evaluate(result);

    delete result.program;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <filename>" << std::endl;
        exit(1);
    }

    try {
        run(argv[1]);
    }
    catch (const char *e) {
        std::cout << e << std::endl;
    }

    return 0;
}
