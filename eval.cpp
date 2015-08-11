/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
