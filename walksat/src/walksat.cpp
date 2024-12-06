#include"walksat.h"
#include<vector>
#include<ctime>
#include<random>
#include<iostream>

using var_t = uint64_t;
using lit_t = int64_t;
using clause_t = std::vector<lit_t>;
using form_t = std::vector<clause_t>;

using value_t = int8_t; // -1, +1
using assign_t = std::vector<value_t>;

bool eval_formula(
    const form_t& formula,
    const assign_t& assign,
    std::vector<uint64_t>& clause_unsat_list,
    std::vector<uint64_t>& var_sat_to_unsat,
    std::vector<uint64_t>& var_unsat_to_sat
) {
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

// solve_formula : use walksat to solve SAT problem
bool solve_formula(
    uint64_t seed, 
    uint64_t max_time_s,
    double rand_var_prob,
    const form_t& formula,
    assign_t& assign
) {
    uint64_t num_variables = assign.size()-1;

    std::uniform_real_distribution<double> dist_float01(0, 1);
    std::default_random_engine engine(seed);

    uint64_t start_time_s = std::time(nullptr);
    {
        // init assignment
        assign[0] = 0;
        for (var_t v=1; v < num_variables+1; v++) {
            if (dist_float01(engine) < 0.5 ) {
                assign[v] = -1;
            } else {
                assign[v] = +1;
            }
        }

        // walksat
        std::vector<uint64_t> clause_unsat_list; // clause_unsat_list: list of unsat clauses
        std::vector<uint64_t> var_sat_to_unsat(num_variables+1); // var_sat_to_unsat[10] = 20 : var 10 makes 20 clauses sat -> unsat
        std::vector<uint64_t> var_unsat_to_sat(num_variables+1); // var_sat_to_unsat[10] = 20 : var 10 makes 20 clauses unsat -> sat

        uint64_t loop_count = 0;
        while (true) {
            loop_count += 1;
            uint64_t time_s = std::time(nullptr);
            if (time_s > start_time_s + max_time_s) {
                std::cout << "timeout: loop_count " << loop_count << std::endl;
                return false;
            }
            // eval formula
            bool sat = eval_formula(formula, assign, clause_unsat_list, var_sat_to_unsat, var_unsat_to_sat);
            if (sat) {
                return true;
            }
            // pick random unsat clause uniformly
            uint64_t c = clause_unsat_list[uint64_t(dist_float01(engine) * clause_unsat_list.size())];
            const clause_t& clause = formula[c];

            var_t flip_var;
            // local search
            if (dist_float01(engine) < rand_var_prob) {
                // with rand_var_prob pick random var uniformly
                flip_var = abs(clause[uint64_t(dist_float01(engine) * clause.size())]);
            } else {
                // pick the var with the most increase in number of sat clauses 
                int64_t best_diff = INT64_MIN;
                var_t best_var = 0;
                for (var_t v=1; v < num_variables+1; v++) {
                    int64_t diff = int64_t(var_unsat_to_sat[v]) - int64_t(var_sat_to_unsat[v]);
                    if (diff > best_diff) {
                        best_diff = diff;
                        best_var = v;
                    }
                }
                flip_var = best_var;
                if (flip_var == 0) {
                    __builtin_trap();
                }
            }

            assign[flip_var] *= -1;
        }
    }

}

uint8_t c_walksat(
    uint64_t seed,
    uint64_t max_time_s,
    double rand_var_prob,
    uint64_t num_variables,
    uint64_t num_clauses,
    int64_t* formula_flatten,
    int8_t* assignment
) {
    // parse formula
    form_t formula;
    {
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
    }

    assign_t assign(num_variables+1);

    bool sat = solve_formula(seed, max_time_s, rand_var_prob, formula, assign);

    for (uint64_t v=0; v < num_variables; v++) {
        assignment[v] = assign[v+1];
    }

    return uint8_t(sat);
}