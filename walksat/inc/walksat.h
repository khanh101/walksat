#ifndef _WALKSAT_H_
#define _WALKSAT_H_
#include<stdint.h>


extern "C" {
    double c_walksat(uint64_t seed, uint64_t max_time_s, double rand_var_prob, uint64_t num_variables, uint64_t num_clauses, int64_t* formula_flatten, double* clause_weight, int8_t* assignment);
}
#endif // _WALKSAT_H_ 