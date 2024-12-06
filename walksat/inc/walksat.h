#ifndef _WALKSAT_H_
#define _WALKSAT_H_
#include<stdint.h>


extern "C" {
    uint8_t c_walksat(uint64_t seed, uint64_t max_time_s, double rand_var_prob, uint64_t num_variables, uint64_t num_clauses, uint64_t* clause_weight, int64_t* formula_flatten, int8_t* assignment);
}
#endif // _WALKSAT_H_ 