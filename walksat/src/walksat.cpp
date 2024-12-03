#include"walksat.h"
#include<vector>
#include<ctime>
#include<random>

const double RAND_VAR_PROB = 0.05;

using var_t = uint64_t;
using lit_t = int64_t;
using clause_t = std::vector<lit_t>;
using form_t = std::vector<clause_t>;
using assign_t = std::vector<int8_t>;


bool eval_formula(const form_t& formula, const assign_t& assign, std::vector<uint64_t>& clause_unsat_list, std::vector<uint64_t>& var_sat_to_unsat, std::vector<uint64_t>& var_unsat_to_sat) {
    clause_unsat_list.clear();
    for (uint64_t v=0; v<var_sat_to_unsat.size(); v++) {
        var_sat_to_unsat[v] = 0;
    }
    for (uint64_t v=0; v<var_unsat_to_sat.size(); v++) {
        var_unsat_to_sat[v] = 0;
    }

    bool global_sat = true;
    for (uint64_t c=0; c<formula.size(); c++) {
        const clause_t& clause = formula[c];
        // process
        bool sat = false;
        var_t uniq_sat_var;
        uint64_t sat_var_count = 0;

        for (uint64_t l=0; l < clause.size(); l++) {
            lit_t lit = clause[l];
            var_t var = abs(lit);
            int8_t val = assign[var];
            if (lit * val > 0) {
                sat = true;
                sat_var_count += 1;
                uniq_sat_var = var;
                if (sat_var_count >= 2) {
                    break;
                }
            }
        }

        // post process
        if (sat and sat_var_count == 1) {
            var_sat_to_unsat[uniq_sat_var] += 1;
        }
        if (not sat) {
            for (uint64_t l=0; l < clause.size(); l++) {
                lit_t lit = clause[l];
                var_t var = abs(lit); 
                var_unsat_to_sat[var] += 1;
            }
            global_sat = false;
            clause_unsat_list.push_back(c);
        }
    }
    return global_sat;
}

uint8_t walksat(uint64_t seed, uint64_t max_time_s, uint64_t num_variables, uint64_t num_clauses, int64_t* formula_flatten, uint8_t* assignment) {
    // parse formula
    form_t formula;
    uint64_t i = 0;
    for (uint64_t c=0; c < num_clauses; c++) {
        clause_t clause;
        while (true) {
            lit_t literal = formula_flatten[i];
            i++;
            if (literal == 0) {
                break;
            }
            clause.push_back(literal);
        }
        formula.push_back(clause);
    }

    // init assignment
    std::uniform_int_distribution<uint8_t> dist_int01(0, 1);
    std::uniform_real_distribution<double> dist_float01(0, 1);
    std::default_random_engine engine(seed);

    assign_t assign(num_variables+1);
    for (var_t v=1; v < num_variables+1; v++) {
        assign[v] = dist_int01(engine);
    }

    // walksat
    uint64_t start_time_s = std::time(nullptr);

    std::vector<uint64_t> clause_unsat_list; // clause_unsat_list: list of unsat clauses
    std::vector<uint64_t> var_sat_to_unsat(num_variables+1); // var_sat_to_unsat[10] = 20 : var 10 makes 20 clauses sat -> unsat
    std::vector<uint64_t> var_unsat_to_sat(num_variables+1); // var_sat_to_unsat[10] = 20 : var 10 makes 20 clauses unsat -> sat
    std::vector<int64_t> var_diff(num_variables+1);

    while (true) {
        uint64_t time_s = std::time(nullptr);
        if (time_s > start_time_s + max_time_s) {
            return 0;
        }
        // eval formula
        bool sat = eval_formula(formula, assign, clause_unsat_list, var_sat_to_unsat, var_unsat_to_sat);
        if (sat) {
            return 1;
        }
        // pick random unsat clause
        uint64_t c = clause_unsat_list[uint64_t(dist_float01(engine) * clause_unsat_list.size())];
        const clause_t& clause = formula[c];

        var_t flip_var;
        if (dist_float01(engine) < RAND_VAR_PROB) {
            // with RAND_VAR_PROB pick random var
            flip_var = abs(clause[uint64_t(dist_float01(engine) * clause.size())]);
        } else {
            // pick best var
            int64_t best_diff = -99999999;
            var_t best_var = 0;
            for (var_t v=1; v < num_variables+1; v++) {
                int64_t diff = int64_t(var_unsat_to_sat[v]) - int64_t(var_sat_to_unsat[v]);
                if (diff > best_diff) {
                    best_diff = diff;
                    best_var = v;
                }
            }
            flip_var = best_var;
        }

        assign[flip_var] *= -1;
    }



    return 23;
}