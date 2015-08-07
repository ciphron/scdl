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
#include <fstream>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <json/json.h>


using namespace scdl;

struct Vars {
    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
};



Variable json_object_to_var(json_object *obj)
{
    Variable var;

    var.name = json_object_get_string(json_object_object_get(obj, "name"));

    std::string type = json_object_get_string(json_object_object_get(obj,
                                                                     "type"));

    if (!type.compare(0, 3, "int")) {
        var.type = VAR_INT;
    }
    else if (!type.compare(0, 4, "uint")) {
        var.type = VAR_UINT;
    }
    else if (type == "bool") {
        var.type = VAR_BOOL;
    }
    else {
        var.type = VAR_BITSTRING;
    }

    struct json_object *comps = json_object_object_get(obj, "components");
    size_t n_comps = json_object_array_length(comps);
    for (int i = 0; i < n_comps; i++) {
        std::string wire =
            json_object_get_string(json_object_array_get_idx(comps,
                                                             i));
        var.components.push_back(wire);
    }

    
    return var;
}

Vars *read_vars_file(std::ifstream &in)
{
    Vars *vars = new Vars;

    std::string json;
    std::string line;
    while (!in.eof()) {
        getline(in, line);
        json += line;
    }

    json_object *obj = json_tokener_parse(json.c_str());

    struct json_object *inputs = json_object_object_get(obj, "inputs");
    size_t n_inputs = json_object_array_length(inputs);
    for (int i = 0; i < n_inputs; i++) {
        json_object *v_obj = json_object_array_get_idx(inputs, i);
        vars->inputs.push_back(json_object_to_var(v_obj));
        free(v_obj);
    }


    struct json_object *outputs = json_object_object_get(obj, "outputs");
    size_t n_outputs = json_object_array_length(outputs);
    for (int i = 0; i < n_outputs; i++) {
        json_object *v_obj = json_object_array_get_idx(outputs, i);
        vars->outputs.push_back(json_object_to_var(v_obj));
        free(v_obj);
    }

    free(inputs);
    free(outputs);
    free(obj);
    
    return vars;
}

void read_variable(const compiler::SCDLProgram *prog,
                   const Variable &var,
                   int *bit_inputs, size_t n_bit_inputs)
{
    std::cout << var.name << " ";

    bool is_signed = false;
    switch (var.type) {
        case VAR_INT:
            is_signed = true;
        case VAR_UINT:
            {
                size_t n_bits = 0;
                size_t n_components = var.components.size();
                scdl::compiler::Variable *prog_vars =
                    new scdl::compiler::Variable[n_components];
                for (int i = 0; i < n_components; i++) {
                    std::string comp = var.components[i];
                    if (!prog->has_variable(comp))
                        throw "Cannot find component input in SCDL program";
                    prog_vars[i] = prog->get_variable(comp);
                    n_bits += prog_vars[i].len;
                }                

                
                uint64_t n;
                uint64_t limit = 1 << ((is_signed) ? (n_bits - 1) : n_bits);
                do {
                    std::cout << "(" << ((is_signed) ? "" : "u")
                              << "int<" << n_bits << ">): ";
                    std::cin >> n;
                }
                while (abs(n) > limit);

                unsigned long v = abs(n);

                // if signed, convert to 2's complement representation
                if (is_signed && n < 0)
                    v = (v ^ ~0) + 1;

                int bit_n = 0;
                for (int component_index = 0; component_index < n_components;
                         component_index++) {
                    scdl::compiler::Variable prog_var = prog_vars[component_index];

                    if (prog_var.input_index >= n_bit_inputs)
                        throw "Input index out of bounds";

                    for (int i = 0; i < prog_var.len; i++) {
                        bit_inputs[prog_var.input_index + i] = v & 1;
                        v >>= 1;
                    }
                }
                delete[] prog_vars;
            }
            break;
        default:
            throw "Unknown type";
    };  

}

void print_variable(const Variable &var,
                    std::map<std::string,int> &wire_bits)
{
    std::cout << var.name << " ";

    bool is_signed = false;
    switch (var.type) {
        case VAR_INT:
            is_signed = true;
        case VAR_UINT:
            {
                const int n_bits = var.components.size();
                std::cout << "(" << ((is_signed) ? "" : "u")
                          << "int<" << n_bits << ">): ";                 

                uint64_t n = 0;
                for (int i = 0; i < n_bits; i++) {
                    if (wire_bits.find(var.components[i]) == wire_bits.end())
                        throw "Cannot find bit of output variable";
                    n |= wire_bits[var.components[i]] << i;
                }

                if (is_signed) {
                    // convert from 2's complement
                    if (n & (1 << (n_bits - 1)))
                        std::cout << '-' << (n ^ ((1 << n_bits) - 1)) + 1;
                    else
                        std::cout << n;
                }
                else
                    std::cout << n;
                std::cout << std::endl;
                break;
            }
        case VAR_BOOL:
            {
                if (var.components.size() != 1) {
                    throw "Number of components for type bool should be 1";
                }
                std::cout << ((wire_bits[var.components[0]]) ? "true" : "false")
                          << std::endl;
                break;
            }
        default:
            throw "Unsupported type";
    };  

}



void run(const std::string &scdl_file)
{
    std::ifstream vars_in((scdl_file + ".vars").c_str());
    if (!vars_in.good()) {
        vars_in.close();
        std::cerr << "No .vars file found" << std::endl;
        return;
    }
    Vars *vars = read_vars_file(vars_in);

    compiler::SCDLProgram *prog =
        compiler::SCDLProgram::compile_program_from_file(scdl_file);
    size_t n_bit_inputs = prog->get_num_variable_inputs();
    int *bit_inputs = new int[n_bit_inputs];
    size_t n_bit_constants = prog->get_num_constants();
    int *bit_constants = new int [n_bit_constants];

    for (int i = 0; i < n_bit_inputs; i++)
        bit_inputs[i] = 0;

    for (int i = 0; i < n_bit_constants; i++)
        bit_constants[i] = prog->get_constant(i).value;

    std::vector<Variable>::iterator itr;
    for (itr = vars->inputs.begin(); itr != vars->inputs.end(); itr++) {
        Variable var = *itr;
        read_variable(prog, var, bit_inputs, n_bit_inputs);
    }

    std::map<std::string,int> wire_bits;
    for (itr = vars->outputs.begin(); itr != vars->outputs.end(); itr++) {
        Variable var = *itr;
        for (int i = 0; i < var.components.size(); i++) {
            std::cout << "Running bit " << i << std::endl;
            // If we have not evaluated the wire yet, evaluate now
            if (wire_bits.find(var.components[i]) == wire_bits.end()) {
                if (!prog->has_circuit(var.components[i])) {
                    throw ("Could not find definition for "
                           + var.components[i]).c_str();
                }
                std::cout << "depth: "
                          << prog->get_circuit(var.components[i])->get_mult_depth()
                          << std::endl;
                int v = prog->run(var.components[i], bit_inputs,
                                   bit_constants) % 2;
                if (v < 2)
                     v = (v + 2) % 2;

                wire_bits[var.components[i]] = v;
            }
        }
        print_variable(var, wire_bits);
    }
    
                                     
    delete(vars);
    delete[] bit_inputs;
    vars_in.close();
    delete prog;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << "<filename>" << std::endl;
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
