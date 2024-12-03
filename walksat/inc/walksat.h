#ifndef _WALKSAT_H_
#define _WALKSAT_H_
#include<stdint.h>


extern "C" {
    // walksat : 
    // cnf : <num_variables> <num_clauses> ...
    // out : binnary string
    uint8_t walksat(uint64_t max_time_s, uint64_t num_variables, uint64_t num_clauses, int64_t* formula, uint8_t* assignment);
}
#endif // _WALKSAT_H_ 