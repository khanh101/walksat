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

double eval_formula(
    const form_t& formula,
    const weight_t& weight,
    const assign_t& assign,
    std::vector<uint64_t>& clause_unsat_list, // list of unsat clauses
    std::vector<double>& var_sat_to_unsat, // flip this var, lose this amount of weight
    std::vector<double>& var_unsat_to_sat // flip this var, gian this amount of weight
) {
    // reset
    clause_unsat_list.clear();
    for (uint64_t v=0; v < var_sat_to_unsat.size(); v++) {
        var_sat_to_unsat[v] = 0.0;
    }
    for (uint64_t v=0; v < var_unsat_to_sat.size(); v++) {
        var_unsat_to_sat[v] = 0.0;
    }

    // eval
    double unsat_weight = 0;
    for (uint64_t c=0; c<formula.size(); c++) {
        const clause_t& clause = formula[c];
        // process
        bool sat = false;
        var_t uniq_sat_var; // the last var making this clause sat
        uint64_t sat_var_count = 0; // number of var making this clause sat

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
            unsat_weight += weight[c];
            clause_unsat_list.push_back(c);
        }
    }
    return unsat_weight;
}


uint64_t weighted_random(const std::vector<double>& dist, double v) {
    double sum_weight;
    for (uint64_t i=0; i<dist.size(); i++) {
        sum_weight += dist[i];
    }
    double r = v * sum_weight;

    uint64_t i = 0;
    while (true) {
        double w = dist[i];
        if (r < w) {
            return i;
        }
        r -= w;
    }
}


// solve_formula : use walksat to solve weighted-MaxSAT problem
double solve_formula(
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
        std::vector<uint64_t> clause_unsat_list;
        std::vector<double> clause_unsat_dist;
        std::vector<double> var_sat_to_unsat(num_variables+1);
        std::vector<double> var_unsat_to_sat(num_variables+1);

        uint64_t best_unsat_weight = std::numeric_limits<uint64_t>::max();
        assign_t best_assign(num_variables+1);

        uint64_t loop_count = 0;
        while (true) {
            loop_count += 1;
            uint64_t time_s = std::time(nullptr);
            if (time_s > start_time_s + max_time_s) {
                std::cout << "timeout: loop_count " << loop_count << std::endl;
                copy_vector<value_t>(assign, best_assign);
                return best_unsat_weight;
            }
            // eval formula
            bool unsat_weight = eval_formula(formula, weight, assign, clause_unsat_list, var_sat_to_unsat, var_unsat_to_sat);
            if (unsat_weight == 0) {
                return 0.0;
            }

            if (unsat_weight < best_unsat_weight) {
                best_unsat_weight = unsat_weight;
                copy_vector<value_t>(best_assign, assign);
            }
            
            // pick random unsat clause uniformly
            // uint64_t c = clause_unsat_list[uint64_t(dist_float01(engine) * clause_unsat_list.size())];
            // const clause_t& clause = formula[c];
            // pick random unsat clause according to weight
            clause_unsat_dist.clear();
            for (uint64_t i=0; i<clause_unsat_list.size(); i++) {
                clause_unsat_dist.push_back(weight[clause_unsat_list[i]]);
            }
            uint64_t i = weighted_random(clause_unsat_dist, dist_float01(engine));
            uint64_t c = clause_unsat_list[i];
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

double c_walksat(
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

    double best_unsat_weight = solve_formula(seed, max_time_s, rand_var_prob, formula, weight, assign);

    for (uint64_t v=0; v < num_variables+1; v++) {
        assignment[v] = assign[v];
    }

    return best_unsat_weight;
}