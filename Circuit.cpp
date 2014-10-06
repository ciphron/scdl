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


/**
 * Store circuit in internal form. Makes a copy of every gate.
 */
bool Circuit::check_well_formed(std::vector<InternalGate> &gates, size_t n_inputs,
                                Gate *current_gate, std::map<Gate*,unsigned int> &visited,
                                unsigned int *gate_index, int depth)
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
    int cur_depth = depth;

    if (current_gate->type == GATE_MULT)
        depth++;
    if (depth > mult_depth)
        mult_depth = depth;


    InternalGate internal;

    internal.input_index = current_gate->input_index;
    internal.type = current_gate->type;
    internal.fan_in = fan_in;
    internal.depth = cur_depth;
    if (fan_in > 0)
        internal.in_gates = new unsigned int[fan_in];
    else
        internal.in_gates = NULL;

    for (int i = 0; i < fan_in && valid; i++) {
        unsigned int index;
        valid = check_well_formed(gates, n_inputs,
                                  current_gate->in_gates[i], visited,
                                  &index, depth);
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

