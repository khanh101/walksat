#cython: language_level=3
cimport libc.stdint as stdint

cdef extern from "walksat.h":
    stdint.uint8_t walksat(stdint.uint64_t seed, stdint.uint64_t max_time_s, stdint.uint64_t num_variables, stdint.uint64_t num_clauses, stdint.int64_t* formula_flatten, stdint.uint8_t* assignment);

import numpy as np

def c_walksat(seed: int, max_time_s: int, formula: list[list[int]]) -> tuple[int, list[int]]:
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

    formula_flatten = []
    for clause in formula:
        for literal in clause:
            formula_flatten.append(literal)
        formula_flatten.append(0)
    formula_flatten_np = np.ascontiguousarray(np.array(formula_flatten, dtype=np.int64))
    assignment_np = np.ascontiguousarray(np.empty(shape=(num_variables,), dtype=np.uint8))

    cdef stdint.uint64_t seed_c = seed
    cdef stdint.uint64_t max_time_s_c = max_time_s
    cdef stdint.uint64_t num_variables_c = num_variables
    cdef stdint.uint64_t num_clauses_c = num_clauses 
    cdef stdint.int64_t[:] formula_flatten_c = formula_flatten_np
    cdef stdint.uint8_t[:] assignment_c = assignment_np

    satisfiable_c = walksat(seed_c, max_time_s_c, num_variables_c, num_clauses_c, &formula_flatten_c[0], &assignment_c[0])

    satisfiable = int(satisfiable_c)

    return satisfiable, [int(a) for a in assignment_np]

