#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <set>
#include <vector>
#include <map>
#include <cstdlib>


/**
 * This is a very cursory version of a circuit evaluator.
 * Currently, this is not parallelized.
 */

#include <iostream>

#define MIN_DEPTH_FOR_PARALLEL 3

enum GateType {
    GATE_MULT,
    GATE_ADD,
    GATE_OUT,
    GATE_IN
};


struct Gate {
    GateType type;
    unsigned int input_index;
    size_t fan_in;
    Gate **in_gates;
};

struct InternalGate {
    GateType type;
    unsigned int input_index;
    size_t fan_in;
    unsigned int *in_gates;
    bool visited;
    int depth;
};



typedef std::set<Gate*> GateSet;

class Circuit {
public:
    Circuit() {n_inputs = 0;}

    Circuit(size_t n_inputs, Gate *output_gate)
        : n_inputs(n_inputs), mult_depth(0) {
        std::map<Gate*,unsigned int> visited;
        if (!check_well_formed(gates, n_inputs, output_gate, visited,
                               &output_gate_index, 0)) {
            free_gates();
            throw "Circuit not well formed";
        }
    }
    ~Circuit() {
        free_gates();
    }

    size_t get_num_inputs() const {
        return n_inputs;
    }

    int get_mult_depth() const {
        return mult_depth;
    }

    template <class T>
        T evaluate(const T *inputs, bool store=false) {
        for (int i = 0; i < gates.size(); i++)
            gates[i].visited = false;

        /* if (store) { */
        /*     T stored[gates.size()]; */
        /*     return eval_gate_with_store(output_gate_index, inputs, stored); */
        /* } */
        /* else */
        return eval_gate_no_store(output_gate_index, inputs);
    }



    /* template <class T> */
    /*     T eval_gate_with_store(unsigned int gate_index, const T *inputs, */
    /*                            T *stored) */
    /*     { */
    /*         InternalGate &gate = gates[gate_index]; */
    /*         if (gate.visited) */
    /*             return stored[gate_index]; */

    /*         if (gate.type == GATE_IN) { */
    /*             T value = inputs[gate.input_index]; */
    /*             stored[gate_index] = value; */
    /*             gate.visited = true; */
    /*             return value; */
    /*         } */

    /*         int fan_in = gate.fan_in; */
    /*         T aggr = eval_gate_with_store(gate.in_gates[0], inputs, stored); */
    /*         for (int i = 1; i < fan_in; i++) { */
    /*             T v = eval_gate_with_store(gate.in_gates[i], inputs, stored); */
    /*             if (gate.type == GATE_MULT) */
    /*                 aggr *= v; */
    /*             else if (gate.type == GATE_ADD) */
    /*                 aggr += v; */
    /*         } */
    /*         stored[gate_index] = aggr; */
    /*         gate.visited = true; */
    /*         return aggr; */
    /*     } */

    template <class T> 
        T eval_gate_no_store(unsigned int gate_index, const T *inputs)
    {
            InternalGate &gate = gates[gate_index];

            if (gate.type == GATE_IN) {
                T value = inputs[gate.input_index];
                gate.visited = true;
                return value;
            }

            int fan_in = gate.fan_in;

            T aggr = eval_gate_no_store(gate.in_gates[0], inputs);
            for (int i = 1; i < fan_in; i++) {
                T v = eval_gate_no_store(gate.in_gates[i], inputs);
                if (gate.type == GATE_MULT)
                    aggr *= v;
                else if (gate.type == GATE_ADD)
                    aggr += v;
            }

            gate.visited = true;
            return aggr;
        }



 private:
    unsigned int output_gate_index;
    std::vector<InternalGate> gates;
    size_t n_inputs;
    int mult_depth;

    bool check_well_formed(std::vector<InternalGate> &gates, size_t n_inputs,
                           Gate *current_gate, std::map<Gate*,unsigned int> &visited,
                           unsigned int *gate_index, int depth);
    
    void free_gates();

    
};

Gate input_gate(unsigned int index);
Gate operator_gate(GateType type, Gate *in1, Gate *in2);

Gate *new_input_gate(unsigned int index);
Gate *new_operator_gate(GateType type, Gate *in1, Gate *in2);


#endif // CIRCUIT_H
