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

#include <iostream>


#include <algorithm>

namespace scdl {

void Circuit::count_gates()
{
    n_add_gates = 0;
    n_mult_gates = 0;
    for (int i = 0; i < gates.size(); i++)
        gates[i].visited = false;
    count_gates_rec(output_gate_index);
    
}

void Circuit::count_gates_rec(unsigned int gate_index)
{
    InternalGate &gate = gates[gate_index];
    if (gate.visited)
        return;

    gate.visited = true;

    if (gate.type == GATE_MULT)
        n_mult_gates++;
    else if (gate.type == GATE_ADD)
        n_add_gates++;

    for (int i = 0; i < gate.fan_in; i++)
        count_gates_rec(gate.in_gates[i]);    
}
    
int Circuit::compute_depth()
{
    for (int i = 0; i < gates.size(); i++) {
        gates[i].visited = false;
        gates[i].depth = 0;
    }

    return compute_depth_rec(output_gate_index, 0);
}

int Circuit::compute_depth_rec(unsigned int gate_index, int depth) {
    InternalGate &gate = gates[gate_index];

    if (gate.visited) {
        return depth + gate.depth;
    }

    gate.visited = true;
    
    if (gate.type == GATE_IN) {
        gate.depth = 0;
        return depth;
    }

    if (gate.type == GATE_MULT) {
        depth++;
    }

    int fan_in = gate.fan_in;
    int max_depth = compute_depth_rec(gate.in_gates[0], depth);
    for (int i = 1; i < fan_in; i++) {
        int d = compute_depth_rec(gate.in_gates[i], depth);
        if (d > max_depth)
            max_depth = d;
    }
    gate.depth = max_depth - depth;

    return max_depth;
}

    
bool Circuit::check_well_formed(std::vector<InternalGate> &gates,
                                size_t n_inputs,
                                Gate *current_gate,
                                std::map<Gate*,unsigned int> &visited,
                                unsigned int *gate_index)
{
    if (visited.find(current_gate) != visited.end()) {
        *gate_index = visited[current_gate];
        return true;
    }

    if (current_gate->type == GATE_IN) {
        if (current_gate->input_index > n_inputs)
            return false;
    }
    else if (current_gate->fan_in == 0) 
        return false;

    bool valid = true;
    size_t fan_in = current_gate->fan_in;

    InternalGate internal;

    internal.input_index = current_gate->input_index;
    internal.type = current_gate->type;
    internal.fan_in = fan_in;
    if (fan_in > 0)
        internal.in_gates = new unsigned int[fan_in];
    else
        internal.in_gates = NULL;

    for (int i = 0; i < fan_in && valid; i++) {
        unsigned int index;
        valid = check_well_formed(gates, n_inputs,
                                  current_gate->in_gates[i], visited,
                                  &index);
        if (valid) {
            internal.in_gates[i] = index;
        }
    }


    if (valid) {
        *gate_index = gates.size();
        visited[current_gate] = *gate_index;
        gates.push_back(internal);
    }

    return valid;
}




Gate input_gate(unsigned int index)
{
    Gate g;

    g.type = GATE_IN;
    g.input_index = index;
    g.fan_in = 0;
    g.in_gates = NULL;

    return g;
}

Gate operator_gate(GateType type, Gate *in1, Gate *in2)
{
    Gate g;

    g.type = type;
    g.fan_in = 2;
    g.in_gates = new Gate*[2];
    g.in_gates[0] = in1;
    g.in_gates[1] = in2;

    return g;
}

Gate *new_input_gate(unsigned int index)
{
    Gate *g = new Gate;

    g->type = GATE_IN;
    g->input_index = index;
    g->fan_in = 0;
    g->in_gates = NULL;

    return g;
}

Gate *new_operator_gate(GateType type, Gate *in1, Gate *in2)
{
    Gate *g = new Gate;

    g->type = type;
    g->fan_in = 2;
    g->in_gates = new Gate*[2];
    g->in_gates[0] = in1;
    g->in_gates[1] = in2;

    return g;
}


void Circuit::free_gates()
{
    // free allocated memory
    for (int i = 0; i < gates.size(); i++) {
        if (gates[i].fan_in > 0 && gates[i].in_gates != NULL) {            
            delete[] gates[i].in_gates;
            gates[i].in_gates = NULL;
        }
    }
}

}
