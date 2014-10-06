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


#ifndef SCDL_PROGRAM_H
#define SCDL_PROGRAM_H

#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include "Circuit.h"

struct Constant {
    int value;
    unsigned int input_index;
};

struct Variable {
    size_t len;
    unsigned int input_index;
};


class SCDLProgram {
 public:
    
    Variable get_variable(const std::string &var_name) const;
    Constant get_constant(const std::string &const_name) const;
    Constant get_constant(unsigned int constant_no) const;
    std::string get_variable_name(unsigned int input_index) const;
    size_t get_num_variables() const;
    size_t get_num_variable_inputs() const;
    Circuit *get_circuit() const;
    bool has_input_variable(const std::string &var_name) const;
    size_t get_num_constants() const;
    bool has_constant(const std::string &const_name) const;
    std::string get_constant_name(unsigned int constant_no) const;

    template <class T>
    T run(T *var_inputs, T *constants) {
        std::vector<T> inputs;
        std::map<std::string,Constant>::iterator itr;

        for (int i = 0; i < n_var_inputs; i++)
            inputs.push_back(var_inputs[i]);

        for (int i =0; i < const_names.size(); i++) {            
            Constant constant = const_map[const_names[i]];
           
            inputs.push_back(constants[i]);
        }

        return circuit->evaluate(&inputs[0]);
    }
        
    
    static SCDLProgram *compile_program_from_file(std::string file_name);

 protected:
    SCDLProgram(Gate *output_gate,
                std::map<std::string,Variable> &var_map,
                std::map<std::string,Constant> &const_map);
    ~SCDLProgram();



 private:
    Circuit *circuit;
    std::map<std::string,Constant> const_map;
    std::vector<std::string> const_names;
    std::map<std::string,Variable> var_map;
    std::vector<std::string> var_names;
    size_t n_var_inputs;

};


#endif // SCDL_PROGRAM_H
