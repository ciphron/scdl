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

#include "Circuit.h"
#include "common.h"
#include <stack>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "SCDLProgram.h"

#define BUFFER_SIZE 1024

using namespace std;
using namespace boost;

namespace scdl {

/*
 * We use a different namespace here because there may be
 * name conflicts. Strictly speaking this is an interpreter
 * at present but we might be translating to a traget language in the near
 * future.
 */
namespace compiler {

typedef map<string,Gate*> VariableMap;
typedef pair<string,Gate*> VarMapping;


struct InputDesc {
    unsigned int value;
    unsigned int key_index;
};


enum TokenType {
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_OP_MUL,
    TOKEN_OP_ADD,
    TOKEN_OPERAND,
    TOKEN_FUNCTION,
    TOKEN_CIRCUIT,
    TOKEN_ARGUMENT
};

struct FunctionDesc;


struct Token {
    TokenType type;

    Token(TokenType type) : type(type) {}

    Token(Gate *gate) : type(TOKEN_OPERAND), gate(gate) {}

    Token(TokenType type, Gate *gate) : type(type), gate(gate) {}

    Token(TokenType type, FunctionDesc *f, vector<Gate*> args)
       : type(TOKEN_FUNCTION), function(f), args(args) {}

    Token(string arg_name) : type(TOKEN_ARGUMENT), arg_name(arg_name) {}

    Token(TokenType type, string arg_name) : type(type), arg_name(arg_name) {}

    Gate *gate;
    FunctionDesc *function;
    vector<Gate*> args;
    string arg_name;

};

struct FunctionDesc {
    string name;
    vector<string> params;
    list<Token> tokens;
};


struct Operation {
    Gate *left;
    Gate *right;
    GateType op;

    bool operator<(const Operation &oper) const {
        if (left < oper.left)
            return true;
        if (left == oper.left && right < oper.right)
            return true;
        if (left == oper.left && right == oper.right && op < oper.op)
            return true;
        return false;
    }
};



typedef map<string,FunctionDesc*> FunctionDescMap;
typedef pair<string,FunctionDesc*> FuncMapping;

/* Building-block functions for parsing programs */

void instantiate_function(list<Token> &output,
                          FunctionDesc *f,
                          map<string,FunctionDesc*> bindings);
int parse_array_index(string expr, int pos, unsigned int *index);
int parse_function_call(string expr, int pos, vector<string> &arg_exprs);
string print_tokens(const list<Token> &tokens);
string array_name(const string &aname, unsigned int index);
bool is_special_char(char c);
bool is_operator(Token token);
void print_stack(std::stack<Token> stack);
void print_circuit(Gate *gate);

struct Function {
    string name;
    vector<string> params;
    Gate *output_gate;
};


enum SymbolType {
    SYM_FUNCTION,
    SYM_VARIABLE,
    SYM_CONSTANT
};
        

struct SymbolInfo {
    size_t len;
    Gate **gates;
    string name;
    SymbolType type;
    int constant_value;
    FunctionDesc *func;
};

typedef map<string,SymbolInfo> SymbolTable;
typedef pair<string,SymbolInfo> SymbolMapping;


class Compilation {
public:
    Compilation(std::istream &is);
    ~Compilation();

    bool run();
    bool is_finished();

    void fill_variable_info(map<string,Variable> &name_to_index);
    void fill_constant_info(map<string,Constant> &name_to_constant);
    void fill_function_info(map<string,Function> &name_to_function);

private:
    bool compile(std::istream &is);
    FunctionDesc *parse_function(string expr, const vector<string> &params);
    FunctionDesc *read_function(string str);
    void add_new_function(FunctionDesc *desc, Gate *gate=NULL);
    void add_new_variable(string name, size_t len=1);
    void add_new_function(string name, FunctionDesc *desc);
    void add_new_constant(string name, unsigned int value, size_t len=1);
    void add_new_variable(string name, size_t len, unsigned int index);
    Gate *alloc_input_gate(unsigned int input_index);
    Gate *alloc_operator_gate(GateType type, Gate *left, Gate *right);
    Gate *build_circuit_from_rpn_rec(list<Token> &tokens);
    Gate *build_circuit_from_rpn(const list<Token> &tokens);

    bool finished;
    map<Operation,Gate*> operations;
    vector<Gate*> allocated_gates;
    std::istream &is;
    SymbolTable sym_table;
    size_t num_inputs;
    size_t num_constants;
    size_t num_functions;
};

/* 
 * #########################################################
 * Definition of methods in class SCDLProgram
 * #########################################################
 */

SCDLProgram::SCDLProgram(map<string,Gate*> &func_gates,
                         map<string,Variable> &var_map,
                         map<string,Constant> &const_map) 
    : var_map(var_map), var_names(var_map.size()), const_map(const_map) {

    map<string,Variable>::iterator itr;
    int i = 0;
    n_var_inputs = 0;
    for (itr = var_map.begin(); itr != var_map.end(); itr++) {
        var_names[i++] = (itr->first);
        n_var_inputs += itr->second.len;
    }

    size_t n_inputs = n_var_inputs;

    map<string,Constant>::iterator citr;
    for (citr = const_map.begin(); citr != const_map.end(); citr++) {
        const_names.push_back(citr->first);
        n_inputs++;
    }
    map<string,Gate*>::iterator fitr;
    for (fitr = func_gates.begin(); fitr != func_gates.end(); fitr++) {
        circuit_map[fitr->first] = new Circuit(n_inputs, fitr->second);
        circuit_names.push_back(fitr->first);
    }
}

SCDLProgram::~SCDLProgram() {
    map<string,Circuit*>::iterator itr;
    for (itr = circuit_map.begin(); itr != circuit_map.end(); itr++) {
        delete itr->second;
    }
    circuit_map.clear();
}

bool SCDLProgram::has_variable(const string &var_name) const
{
    return var_map.find(var_name) != var_map.end();
}


Variable SCDLProgram::get_variable(const string &var_name) const
{
    return var_map.at(var_name);
}

Constant SCDLProgram::get_constant(const string &const_name) const
{
    return const_map.at(const_name);
}

Constant SCDLProgram::get_constant(unsigned int constant_no) const
{
    return const_map.at(const_names[constant_no]);
}



string SCDLProgram::get_variable_name(unsigned int input_index) const
{
    return var_names[input_index];
}

string SCDLProgram::get_constant_name(unsigned int constant_no) const
{
    return const_names[constant_no];
}

size_t SCDLProgram::get_num_constants() const {
    return const_map.size();
}

bool SCDLProgram::has_constant(const string &const_name) const
{
    return const_map.find(const_name) != const_map.end();
}

bool SCDLProgram::has_input_variable(const string &var_name) const
{
    return var_map.find(var_name) != var_map.end();
}

size_t SCDLProgram::get_num_variables() const
{
    return var_names.size();
}

size_t SCDLProgram::get_num_variable_inputs() const
{
    return n_var_inputs;
}

Circuit *SCDLProgram::get_circuit(const std::string &circuit_name) const
{
    return circuit_map.at(circuit_name);
}

vector<string>::const_iterator SCDLProgram::get_circuit_names() const
{
    return circuit_names.begin();
}


bool SCDLProgram::has_circuit(const std::string &circuit_name) const
{
    return circuit_map.find(circuit_name) != circuit_map.end();
}

size_t SCDLProgram::get_num_circuits() const
{
    return circuit_map.size();
}

SCDLProgram *SCDLProgram::compile_program_from_stream(std::istream &is)
{
    Compilation compilation(is);
    compilation.run();

    map<string,Function> name_to_function;
    map<string,Gate*> gate_map;
    compilation.fill_function_info(name_to_function);

    map<string,Function>::iterator itr;
    for (itr = name_to_function.begin(); itr != name_to_function.end();
         itr++) {
        Function f = itr->second;
        if (f.params.size() == 0)
            gate_map[itr->first] = f.output_gate;
    }
        
    // if (name_to_function.find("out") == name_to_function.end())
    //     throw "No output circuit defined";
    // Gate *output_gate = name_to_function["out"].output_gate;
    map<string,Variable> name_to_variable;
    map<string,Constant> name_to_constant;

    compilation.fill_variable_info(name_to_variable);
    compilation.fill_constant_info(name_to_constant);

    return new SCDLProgram(gate_map, name_to_variable, name_to_constant);;
}

SCDLProgram *SCDLProgram::compile_program_from_file(string file_name)
{
    std::ifstream is(file_name.c_str());

    compile_program_from_stream(is);
    is.close();
}


/* 
 * #########################################################
 * Definition of methods in class Compilation
 * #########################################################
 */

void Compilation::fill_variable_info(map<string,Variable> &name_to_index)
{
    // fill variable info
    for (SymbolTable::iterator itr = sym_table.begin(); itr != sym_table.end();
         itr++) {
        SymbolInfo sym = itr->second;
        if (sym.type == SYM_VARIABLE) {
            Variable v;
            v.input_index = sym.gates[0]->input_index;
            v.len = sym.len;
            name_to_index[itr->first] = v;
        }
    }
}

void Compilation::fill_constant_info(map<string,Constant> &name_to_constant)
{
    // fill constant info
    for (SymbolTable::iterator itr = sym_table.begin(); itr != sym_table.end();
         itr++) {
        SymbolInfo sym = itr->second;
        if (sym.type == SYM_CONSTANT) {
            Constant c;
            c.input_index = sym.gates[0]->input_index;
            c.value = sym.constant_value;
            name_to_constant[itr->first] = c;
        }
    }
}

void Compilation::fill_function_info(map<string,Function> &name_to_function)
{
    // fill function info
    for (SymbolTable::iterator itr = sym_table.begin(); itr != sym_table.end();
         itr++) {
        SymbolInfo sym = itr->second;
        if (sym.type == SYM_FUNCTION) {
            Function f;
            f.name = itr->first;
            f.params = sym.func->params;
            f.output_gate = sym.gates[0];
            name_to_function[itr->first] = f;
        }
    }
}





Gate *Compilation::build_circuit_from_rpn_rec(list<Token> &tokens)
{
    if (tokens.empty())
        return NULL;

    Token token = tokens.back();
    tokens.pop_back();

    if (token.type == TOKEN_OPERAND)
        return token.gate;
    else if (token.type == TOKEN_CIRCUIT)
        return token.gate;                                             
    else if (is_operator(token.type)) {
        Gate *right = build_circuit_from_rpn_rec(tokens);
        if (right == NULL)
            return NULL;

        Gate *left = build_circuit_from_rpn_rec(tokens);
        if (left == NULL)
            return NULL;

        Operation oper;
        oper.left = left;
        oper.right = right;
        oper.op = (token.type == TOKEN_OP_MUL) ? GATE_MULT : GATE_ADD;

        if (operations.find(oper) == operations.end()) {        
            Gate *op_gate = alloc_operator_gate((token.type == TOKEN_OP_MUL)
                                                ? GATE_MULT : GATE_ADD,
                                                left, right);
            operations.insert(pair<Operation,Gate*>(oper, op_gate));
            return op_gate;
        }
        
        return operations[oper];
    }

    return NULL;
}

Gate *Compilation::build_circuit_from_rpn(const list<Token> &tokens)
{
    list<Token> tokens_copy = tokens;

    return build_circuit_from_rpn_rec(tokens_copy);
}


Compilation::Compilation(std::istream &is)
    : is(is), finished(false), num_inputs(0), num_constants(0),
      num_functions(0)
{
}

Compilation::~Compilation()
{
    vector<Gate*>::iterator itr;
    for (itr = allocated_gates.begin(); itr != allocated_gates.end(); itr++)
        delete *itr;
    for (SymbolTable::iterator sitr = sym_table.begin();
         sitr != sym_table.end(); sitr++) {
        if (sitr->second.gates != NULL) {
            delete[] sitr->second.gates;
            sitr->second.gates = NULL;
        }
        if (sitr->second.type == SYM_FUNCTION)
            delete sitr->second.func;
    }
}

/*
 * TODO: This methods badly needs tidyng up and refactoring.
 * Also control flow changes (avoidance of continue) are needed.
 */
FunctionDesc *Compilation::parse_function(string expr,
                                          const vector<string> &params)
{
    expr += ";";

    string cur_sym_name;

    stack<Token> stack;
    list<Token> output;
    int pos = 0;

    /* Here we run the shunting yard algorithm to make an RPN queue (output) */
    while (pos < expr.length()) {
        if (expr[pos] == ' ' || expr[pos] == '\n') {
            pos++;
            continue;
        }
        char t = expr[pos];

        if (is_special_char(t) && cur_sym_name != "") {
            unsigned int ind;
            if (t == '[') {
                int newpos = parse_array_index(expr, pos, &ind);
                string name = array_name(cur_sym_name, ind);
                if (find(params.begin(), params.end(), name) != params.end()) {
                    output.push_back(Token(TOKEN_ARGUMENT, name));
                    pos = newpos;
                    cur_sym_name = "";
                    continue;
                }
            }
            
            if (find(params.begin(), params.end(), cur_sym_name) !=
                     params.end()) {
                // Handle argument
                output.push_back(Token(TOKEN_ARGUMENT, cur_sym_name));       
            }
            else if (sym_table.find(cur_sym_name) != sym_table.end()) {
                // Symbol exists
                SymbolInfo sym = sym_table[cur_sym_name];
                switch (sym.type) {
                    case SYM_VARIABLE:
                        if (sym.len > 1) {
                            if (t != '[')
                                throw "Expected [";

                            // we decrement returned pos because it is
                            // incremented below
                            pos = parse_array_index(expr, pos, &ind) - 1;
                            if (ind >= sym.len)
                                throw "Index out of range";
                            output.push_back(Token(TOKEN_OPERAND,
                                                   sym.gates[ind]));
                        }
                        else {
                            output.push_back(Token(TOKEN_OPERAND,
                                                   sym.gates[0]));
                        }
                        break;
                    case SYM_CONSTANT:
                        output.push_back(Token(TOKEN_OPERAND, sym.gates[0]));
                        break;
                    case SYM_FUNCTION:
                        FunctionDesc *f = sym.func;
                        if (f->params.size() != 0) {
                            if (t != '(')
                                throw "FunctionDesc expects arguments";
                            vector<string> pre_arg_exprs;
                            vector<string> arg_exprs; 
                            pos = parse_function_call(expr, ++pos,
                                                      pre_arg_exprs);
                            for (int i = 0; i < pre_arg_exprs.size(); i++) {
                                string arg = pre_arg_exprs[i];
                                trim(arg);
                                if (sym_table.find(arg) != sym_table.end()) {
                                    SymbolInfo sym = sym_table[arg];
                                    if (sym.type == SYM_VARIABLE &&
                                            sym.len > 1) {
                                        for (int j = 0; j < sym.len; j++) {
                                            string name = array_name(arg, j);
                                            arg_exprs.push_back(name);
                                        }
                                    }
                                    else
                                        arg_exprs.push_back(arg);
                                }
                                else
                                    arg_exprs.push_back(arg);
                            }
                            // Parse args strings
                            if (arg_exprs.size() != f->params.size()) {
                                throw "Incorrect number of arguments passed "
                                      "to function";
                            }
                            map<string,FunctionDesc*> args;
                            for (int i = 0; i < arg_exprs.size(); i++) {
                                FunctionDesc *g = parse_function(arg_exprs[i],
                                                                 params);
                                args[f->params[i]] = g;
                            }
                            instantiate_function(output, f, args);

                            pos++; // point to next char for next iteration
                            cur_sym_name = "";
                            continue;
                        }
                           
                        else {
                            output.push_back(Token(TOKEN_CIRCUIT,
                                                   sym.gates[0]));
                        }
                        break;
                }
            }
            else {
                // Assume it is a new variable
                add_new_variable(cur_sym_name);
                Gate *g = sym_table[cur_sym_name].gates[0];
                output.push_back(Token(g));
            }

            cur_sym_name = "";
        }

        if (t == '(') {
            stack.push(Token(TOKEN_LEFT_PAREN));
        }
        else if (t == ')') {
            while (!stack.empty() && stack.top().type != TOKEN_LEFT_PAREN) {
                Token token = stack.top();
                
                output.push_back(token);
                stack.pop();
            }
            if (stack.empty())
                throw "Mismatched parenthesis";
            else
                stack.pop(); // pop left parenthesis
        }
        else if (t == '*' || t == '+') {
            Token op = Token((t == '*') ? TOKEN_OP_MUL : TOKEN_OP_ADD);

            while (!stack.empty() && is_operator(stack.top())) {
                output.push_back(stack.top());
                stack.pop();
            }
            stack.push(op);
        }
        else if (t != '[') {
            cur_sym_name += t;
        }

        pos++;
    }
    while (!stack.empty()) {
        Token token = stack.top();
        stack.pop();

        if (token.type == TOKEN_LEFT_PAREN)
            throw "Mismatched parenthesis";

        output.push_back(token);
    }
    FunctionDesc *f = new FunctionDesc;
    f->tokens = output;
    f->params = params;

    return f;
}

FunctionDesc *Compilation::read_function(string str)
{
    vector<string> parts;            

    split(parts, str, is_any_of("="));
    if (parts.size() < 2)
        throw "Invalid syntax for function definition";

    vector<string> params;
    trim(parts[0]);
    int pos = parts[0].find_first_of("(");
    if (pos == string::npos) {
        pos = parts[0].find_first_of("=");
    }
    else {
        if (parts[0][parts[0].length() - 1] != ')')
            throw "Invalid syntax for function definition";
        string params_list =
            parts[0].substr(pos + 1,
                            (parts[0].length() - 1) - pos - 1);
        vector<string> pre_params;
        split(pre_params, params_list, is_any_of(","));
        for (int i = 0; i < pre_params.size(); i++) {
            trim(pre_params[i]);
            vector<string> comps;

            // Check whether param is a vector
            // split into components (comps) separated by ":"
            split(comps, pre_params[i], is_any_of(":"));
            if (comps.size() > 1) {
                unsigned int vector_len = read_numeric<unsigned int>(comps[1]);
                for (int j = 0; j < vector_len; j++) {
                    params.push_back(array_name(comps[0], j));
                }
            }
            else
                params.push_back(comps[0]);
        }
    }
    string name = parts[0].substr(0, pos);

    trim(parts[1]);
    FunctionDesc *f = parse_function(parts[1], params);
    f->name = name;
    
    return f;
}

Gate *Compilation::alloc_input_gate(unsigned int input_index)
{
    Gate *g = new_input_gate(input_index);
    allocated_gates.push_back(g);
    return g;
}

void Compilation::add_new_function(FunctionDesc *desc, Gate *gate)
{
    SymbolInfo sym;
    
    sym.name = desc->name;
    sym.gates = new Gate*[1];
    sym.gates[0] = gate;
    sym.type = SYM_FUNCTION;
    sym.func = desc;

    sym_table[desc->name] = sym;
    num_functions++;
}


void Compilation::add_new_variable(string var_name, size_t len,
                                   unsigned int index)
{
    SymbolInfo sym;
    
    sym.name = var_name;
    sym.gates = new Gate*[len];
    for (int i = 0; i < len; i++)
        sym.gates[i] = alloc_input_gate(index + i);
    sym.len = len;
    sym.type = SYM_VARIABLE;
    sym_table[var_name] = sym;

    num_inputs += len;
}


void Compilation::add_new_variable(string var_name, size_t len)
{
    add_new_variable(var_name, len, num_inputs);
}

void Compilation::add_new_constant(string name, unsigned int value, size_t len)
{
    SymbolInfo sym;
    
    sym.name = name;
    sym.len = len;
    sym.type = SYM_CONSTANT;
    sym.constant_value = value;
    sym.gates = new Gate*[1];
    sym.gates[0] = alloc_input_gate(num_constants);
    num_constants++;

    sym_table[name] = sym;
}



Gate *Compilation::alloc_operator_gate(GateType type, Gate *left, Gate *right)
{
    Gate *g = new_operator_gate(type, left, right);
    allocated_gates.push_back(g);
    return g;
}

bool Compilation::run()
{
    if (finished)
        return false;

    finished = compile(is);
    return finished;
}

bool Compilation::compile(std::istream &is)
{
    char buf[BUFFER_SIZE];
    int count;

    while (!is.eof()) {
        bool more = true;
        string stmnt;
        while (!is.eof() && more) {
            string line;
            getline(is, line);
            trim(line);
            more = (line[line.length() - 1] == '\\');
            if (more)
                line.erase(line.end() - 1);
            stmnt += line;
        }

        if (stmnt.empty())
            continue;

        /* Ignore comment - marked by # */
        int first_c_pos = stmnt.find_first_not_of(" \t");
        if (first_c_pos != string::npos && stmnt[first_c_pos] == '#')
            continue;

        /*
         * TODO: Common code for "input" and "constant" should be factored out
         */
        vector<string> parts;
        split(parts, stmnt, is_any_of(" \t"));
        if (parts[0] == "input") {
            // input declaration
            if (parts.size() < 2) 
                throw "Illegal input declaration";
            string rhs;
            for (int i = 1; i < parts.size(); i++)
                rhs += parts[i];

            unsigned int index = num_inputs; // default value
            size_t len = 1;
            
            vector<string> decl_parts;
            split(decl_parts, rhs, is_any_of(":"));
            string var_name = decl_parts[0];
            trim(var_name);

            if (sym_table.find(var_name) != sym_table.end())
                throw "Symbol " + var_name  +  " already declared";

            if (decl_parts.size() > 1) {
                string len_str = decl_parts[1];
                trim(len_str);

                len = read_numeric<unsigned int>(len_str);
            }
            add_new_variable(var_name, len, index);
        }
        else if (parts[0] == "constant") {
                        // input declaration
            if (parts.size() < 2) 
                throw "Illegal input declaration";
            vector<string> decl_parts;            
            string rhs;
            for (int i = 1; i < parts.size(); i++)
                rhs += parts[i];

            split(decl_parts, rhs, is_any_of("="));
            if (decl_parts.size() < 2)
                throw "Illegal constant declaration";

            string constant_name = decl_parts[0];
            trim(constant_name);
            string value_str = decl_parts[1];
            trim(value_str);

            if (sym_table.find(constant_name) != sym_table.end())
                throw "Symbol " + constant_name  +  " already declared";
            int value = read_numeric<int>(value_str);

            /* Assign constants a temorary input index,
             * we later add on to this index the number of "variable" inputs.
             */
            add_new_constant(constant_name, value);
        }
        else if (parts[0] == "include") {
            if (parts.size() != 2)
                throw "File must be specified in 'include' statement";
            trim(parts[1]);
            if (parts[1][0] != '"' || parts[1][parts[1].length() - 1] != '"')
                throw "File name must be specified within quotes (\")";
            string fname = parts[1].substr(1, parts[1].length() - 2);
            ifstream include_is(fname.c_str());
            if (!compile(include_is))
                throw "Failed to load program in file " + fname;
        }
        else if (parts[0] == "func") {            
            string expr = stmnt.substr(parts[0].length(),
                                           stmnt.length() - parts[0].length());
            FunctionDesc *func = read_function(expr);
            if (func->params.size() == 0) {
                // translate  function to circuit
                Gate *gate = build_circuit_from_rpn(func->tokens);
                add_new_function(func, gate);
            }
            else
                add_new_function(func);           
        }
    }

    SymbolTable::iterator itr;
    for (itr = sym_table.begin(); itr != sym_table.end(); itr++) {
        SymbolInfo &sym = itr->second;
        if (sym.type == SYM_CONSTANT)
            sym.gates[0]->input_index += num_inputs;
    }

    return true;
}

/* 
 * #########################################################
 * Useful functions used above
 * #########################################################
 */

inline bool is_special_char(char c)
{
    return c == '(' || c == ')' || c == '*' || c == '+' || c == ';' ||
           c == '[' || c == ']';
}

inline bool is_operator(Token token)
{
    return token.type == TOKEN_OP_ADD || token.type == TOKEN_OP_MUL;
}


void print_stack(stack<Token> stack)
{
    while (!stack.empty()) {
        Token token = stack.top();
        printf("%d\n", token.type);
        stack.pop();
    }
    printf("------------\n");
}



void print_circuit(Gate *gate)
{
    if (gate->type == GATE_IN) {
        printf("<INPUT %d>", gate->input_index);
    }
    else {
        printf("%s", (gate->type == GATE_MULT) ? "*" : "+");
        printf(" [ ");
        for (int i = 0; i < gate->fan_in; i++) {
            print_circuit(gate->in_gates[i]);
            printf(" ");
        }
        printf("]");
    }
}
string print_tokens_rec(list<Token> &tokens)
{
    if (tokens.empty())
        return "";

    Token token = tokens.back();
    tokens.pop_back();

    if (token.type == TOKEN_OPERAND) {
        string out = "<INPUT (";
        char buf[100];
        sprintf(buf, "%d", token.gate->input_index);
        out += buf;
        out += ")> ";
        return out;
    }
    else if (token.type == TOKEN_CIRCUIT) {
        printf("<CIRCUIT>");
    }
    else if (token.type == TOKEN_ARGUMENT) {
        return string("<ARGUMENT (") + token.arg_name + ")> ";
    }
    else if (is_operator(token.type)) {
        string out = (token.type == TOKEN_OP_MUL) ? "* [ " : " + [ ";
        string right = print_tokens_rec(tokens);
        string left = print_tokens_rec(tokens);
        out += left;
        out += right;
        out += "]";

        return out;
    }

    return "";
}

string array_name(const string &aname, unsigned int index)
{
    string name = aname;
 
    trim(name);
    name +=  "[" + lexical_cast<string>(index) + "]";

    return name;
}

string print_tokens(const list<Token> &tokens)
{
    list<Token> copy = tokens;

    return print_tokens_rec(copy);
}



void instantiate_function(list<Token> &output,
                          FunctionDesc *f,
                          map<string,FunctionDesc*> bindings)
{
    list<Token>::iterator itr;

    for (itr = f->tokens.begin(); itr != f->tokens.end(); itr++) {
        Token token = *itr;

        if (token.type == TOKEN_ARGUMENT) {
            if (bindings.find(token.arg_name) == bindings.end())
                throw "Argument not bound";
            output.insert(output.end(),
                          bindings[token.arg_name]->tokens.begin(),
                          bindings[token.arg_name]->tokens.end());
        }
        else
            output.push_back(token);
    }
}

int parse_array_index(string expr, int pos, unsigned int *index)
{
    if (expr[pos] != '[')
        throw "Expected [";
    pos++;
    string sel_str = expr.substr(pos);
    int endpos = sel_str.find_first_of("]");

    if (endpos == string::npos)
        throw "Could not find ]";
    *index = read_numeric<unsigned int>(sel_str.substr(0, endpos));

    return pos + endpos + 1;
}

int parse_function_call(string expr, int pos, vector<string> &arg_exprs)
{
    int begin_pos = pos;
    string args_str;
    int paren_depth = 0;

    string current_arg;
    while (pos < expr.length() && (paren_depth != 0 || expr[pos] != ')')) {
        if (expr[pos] == ',' && paren_depth == 0) {
            arg_exprs.push_back(current_arg);
            current_arg = "";
        }
        else {
            if (expr[pos] == '(')
                paren_depth++;
            else if (expr[pos] == ')')
                paren_depth--;
            current_arg += expr[pos];
        }
        pos++;
    }
    if (current_arg != "")
        arg_exprs.push_back(current_arg);
    
    if (expr[pos] != ')') {
        throw "Invalid function call";
    }

    return pos;
}

}
}
