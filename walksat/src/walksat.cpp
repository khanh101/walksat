#include"walksat.h"
#include<vector>
#include<ctime>
#include<random>
#include<limits>
#include<iostream>

using var_t = uint64_t;
using lit_t = int64_t;
using clause_t = std::vector<lit_t>;
using form_t = std::vector<clause_t>;
using weight_t = std::vector<double>;

using value_t = int8_t; // -1, +1
using assign_t = std::vector<value_t>;

template<typename T> void copy_vector(std::vector<T>& target, const std::vector<T>& source) {
    target.clear();
    for (uint64_t i=0; i< source.size(); i++) {
        target.push_back(source[i]);
    }
}

uint64_t eval_formula(
    const form_t& formula,
    const weight_t& weight,
    const assign_t& assign,
    std::vector<uint64_t>& clause_unsat_list,
    std::vector<double>& var_sat_to_unsat,
    std::vector<double>& var_unsat_to_sat
) {
    clause_unsat_list.clear();
    for (uint64_t v=0; v<var_sat_to_unsat.size(); v++) {
        var_sat_to_unsat[v] = 0.0;
    }
    for (uint64_t v=0; v<var_unsat_to_sat.size(); v++) {
        var_unsat_to_sat[v] = 0.0;
    }

    uint64_t num_unsat_clauses = 0;
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
            var_sat_to_unsat[uniq_sat_var] += weight[c];
        }
        if (not sat) {
            for (uint64_t l=0; l < clause.size(); l++) {
                lit_t lit = clause[l];
                var_t var = abs(lit); 
                var_unsat_to_sat[var] += weight[c];
            }
            num_unsat_clauses += 1;
            clause_unsat_list.push_back(c);
        }
    }
    return num_unsat_clauses;
}



// solve_formula : use walksat to solve SAT problem
uint64_t solve_formula(
    uint64_t seed, 
    uint64_t max_time_s,
    double rand_var_prob,
    const form_t& formula,
    const weight_t& weight,
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
        std::vector<double> var_sat_to_unsat(num_variables+1); // var_sat_to_unsat[10] = 20 : var 10 makes 20 clauses sat -> unsat
        std::vector<double> var_unsat_to_sat(num_variables+1); // var_sat_to_unsat[10] = 20 : var 10 makes 20 clauses unsat -> sat

        uint64_t best_num_unsat_clauses = std::numeric_limits<uint64_t>::max();
        assign_t best_assign(num_variables+1);

        uint64_t loop_count = 0;
        while (true) {
            loop_count += 1;
            uint64_t time_s = std::time(nullptr);
            if (time_s > start_time_s + max_time_s) {
                std::cout << "timeout: loop_count " << loop_count << std::endl;
                copy_vector<value_t>(assign, best_assign);
                return best_num_unsat_clauses;
            }
            // eval formula
            bool num_unsat_clauses = eval_formula(formula, weight, assign, clause_unsat_list, var_sat_to_unsat, var_unsat_to_sat);
            if (num_unsat_clauses == 0) {
                return 0;
            }

            if (num_unsat_clauses < best_num_unsat_clauses) {
                best_num_unsat_clauses = num_unsat_clauses;
                copy_vector<value_t>(best_assign, assign);
            }
            
            // pick random unsat clause uniformly
            // uint64_t c = clause_unsat_list[uint64_t(dist_float01(engine) * clause_unsat_list.size())];
            // const clause_t& clause = formula[c];
            // pick random unsat clause according to weight
            double sum_weight = 0; 
            for (uint64_t c=0; c < clause_unsat_list.size(); c++) {
                sum_weight += weight[c];
            }
            double r = dist_float01(engine) * sum_weight;
            uint64_t c = 0;
            while (true) {
                double w = weight[clause_unsat_list[c]];
                if (r < w) {
                    break;
                }
                r -= w;
            }
            const clause_t& clause = formula[c];

            var_t flip_var;
            // local search
            if (dist_float01(engine) < rand_var_prob) {
                // with rand_var_prob pick random var uniformly
                flip_var = abs(clause[uint64_t(dist_float01(engine) * clause.size())]);
            } else {
                // pick the var with the most increase in number of sat clauses 
                double best_diff = - std::numeric_limits<double>::max();
                var_t best_var = 0;
                for (var_t v=1; v < num_variables+1; v++) {
                    double diff = var_unsat_to_sat[v] - var_sat_to_unsat[v];
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

uint64_t c_walksat(
    uint64_t seed,
    uint64_t max_time_s,
    double rand_var_prob,
    uint64_t num_variables,
    uint64_t num_clauses,
    int64_t* formula_flatten,
    double* clause_weight,
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

    weight_t weight(num_clauses);

    for (uint64_t c=0; c < num_clauses; c++) {
        weight[c] = clause_weight[c];
    }

    assign_t assign(num_variables+1);

    uint64_t best_num_unsat_clauses = solve_formula(seed, max_time_s, rand_var_prob, formula, weight, assign);

    for (uint64_t v=0; v < num_variables+1; v++) {
        assignment[v] = assign[v];
    }

    return best_num_unsat_clauses;
}