#cython: language_level=3
cimport libc.stdint as stdint

cdef extern from "walksat.h":
    stdint.uint8_t c_walksat(
        stdint.uint64_t seed,
        stdint.uint64_t max_time_s,
        double rand_var_prob,
        stdint.uint64_t num_variables,
        stdint.uint64_t num_clauses,
        stdint.uint64_t* clause_weight,
        stdint.int64_t* formula_flatten,
        stdint.int8_t* assignment
    );

import numpy as np

def walksat(formula: list[list[int]], weight: list[int] | None = None, seed: int=1234, max_time_s: int=10, rand_var_prob: float=0.05) -> tuple[int, list[int]]:
    """
    [formula] - cnf formula, for example (x_1 ∧ ¬x_2) ∨ (x_2 ∧ x_3) is [[+1, -2], [+2, +3]]
    [seed] - seed for RNG in C
    [max_time_s] - max time for walksat in seconds
    [rand_var_prob] - probability of picking random var

    return:
    [sat] - satisfiable (1:sat, 0:unsat)
    [assignment] - assignment if sat (+1: true, -1: false)
    """
    # check
    for clause in formula:
        for literal in clause:
            assert literal != 0
    # end check
    
    num_clauses = len(formula)
    num_variables = 0
    for clause in formula:
        for literal in clause:
            num_variables = max(num_variables, abs(literal))

    assert num_variables > 0

    if weight is None:
        weight = [1 for _ in range(len(formula))]

    formula_flatten = []
    for clause in formula:
        for literal in clause:
            formula_flatten.append(literal)
        formula_flatten.append(0)
    formula_flatten_np = np.ascontiguousarray(np.array(formula_flatten, dtype=np.int64))
    clause_weight_np = np.ascontiguousarray(np.array(weight, dtype=np.uint64))
    assignment_np = np.ascontiguousarray(np.empty(shape=(num_variables,), dtype=np.int8))

    cdef stdint.uint64_t seed_c = seed
    cdef stdint.uint64_t max_time_s_c = max_time_s
    cdef double rand_var_prob_c = rand_var_prob
    cdef stdint.uint64_t num_variables_c = num_variables
    cdef stdint.uint64_t num_clauses_c = num_clauses 
    cdef stdint.uint64_t[:] clause_weight_c = clause_weight_np
    cdef stdint.int64_t[:] formula_flatten_c = formula_flatten_np
    cdef stdint.int8_t[:] assignment_c = assignment_np

    satisfiable_c = c_walksat(seed_c, max_time_s_c, rand_var_prob_c, num_variables_c, num_clauses_c, &clause_weight_c[0], &formula_flatten_c[0], &assignment_c[0])

    satisfiable = int(satisfiable_c)

    return satisfiable, [int(a) for a in assignment_np]

