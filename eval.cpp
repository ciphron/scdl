#include "Circuit.h"
#include "SCDLProgram.h"
#include "common.h"
#include <iostream>
#include <ctype.h>
#include <stack>
#include <queue>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>




void perform_evaluation(SCDLProgram *circuit, uint8_t *inputs,
                        size_t n_inputs);






void read_inputs_from_file(char *file_name,
                           uint8_t *inputs,
                           SCDLProgram *program)
{

    size_t n_inputs = program->get_num_variable_inputs();

    std::ifstream is(file_name);
    int n_assignments;

    std::string line;
    getline(is, line);

    while (!is.eof()) {
        getline(is, line);

        boost::trim(line);
        if (line.empty())
            continue;

        std::vector<std::string> parts;
        boost::split(parts, line, boost::is_any_of(":="));
                     
        if (parts.size() < 3) {
            throw "Invalid assignment";
        }

        std::string var_name = parts[0];
        boost::trim(var_name);
        
        if (!program->has_input_variable(var_name))
            throw "No such variable";
        
        Variable var = program->get_variable(var_name);
        
        boost::trim(parts[1]);
        if (parts[1][0] == '(' ) {
            int endpos = parts[1].find_first_of(")");
            if (endpos == std::string::npos)
                throw "Invalid assignment, expected )";

            std::vector<std::string> coeffs;
            std::string coeffs_str = parts[1].substr(1, endpos - 1);
            boost::split(coeffs, coeffs_str, boost::is_any_of(","));
            if (coeffs.size() != var.len)
                throw "Length of vector does not match variable length";
            for (int i = 0; i < var.len; i++) {
                inputs[var.input_index + i] = read_numeric<unsigned int>(coeffs[i]);
            }
        }
        else {
            if (var.len != 1)
                throw "Invalid assigment, expected vector";
            unsigned int value = read_numeric<unsigned int>(parts[1]);
           inputs[var.input_index] = value;
        }
    }


    is.close();
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "usage: main scdl_file assignment_file" << std::endl;
        exit(1);
    }

    try {
        SCDLProgram *prog = SCDLProgram::compile_program_from_file(argv[1]);

        size_t n_inputs = prog->get_num_variable_inputs();

        uint8_t inputs[n_inputs];

        /* Initialize the values of inputs to 0 */
        for (int i = 0; i < n_inputs; i++) {
            inputs[i] = 0;
        }

        read_inputs_from_file(argv[2], inputs, prog);

        
        perform_evaluation(prog, inputs, n_inputs);
    }
    catch (const char *e) {
        std::cerr << "ERROR (reading from files) " << e << std::endl;
    }
}    

void perform_evaluation(SCDLProgram *program, uint8_t *inputs,
                        size_t n_inputs)
{

    uint8_t pl_constants[program->get_num_constants()];
    for (int i = 0; i < program->get_num_constants(); i++) {
        Constant c = program->get_constant(i);
        pl_constants[i] = (uint8_t)c.value;
    }

    uint8_t output = program->run(inputs, pl_constants) % 2;

    std::cout <<  "Result is " << (int)output << std::endl;
}
