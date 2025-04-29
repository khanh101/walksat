#include"walksat.h"
#include<vector>
#include<ctime>
#include<random>
#include<limits>
#include<functional>
#include<iostream>

using rand_t = std::function<double()>;

template<typename T> void copy_vector(std::vector<T>& target, const std::vector<T>& source) {
    target.clear();
    for (uint64_t i=0; i< source.size(); i++) {
        target.push_back(source[i]);
    }
}

uint64_t weighted_random(const std::vector<double>& dist, double v) {
    double sum_weight = 0;
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
        i++;
    }
}

using var_t = uint64_t;
using lit_t = int64_t;
using clause_t = std::vector<lit_t>;
using weight_t = double;

using val_t = int8_t; // 0, -1, +1
using assign_t = std::vector<val_t>;

struct problem {
    std::vector<clause_t> clause_list;
    std::vector<weight_t> weight_list;
    uint64_t num_variables;
};

struct solution {
    assign_t assignment;
    weight_t assignment_weight; // weighted sum of unsat clauses
    std::vector<uint64_t> clause_unsat_idx_list; // list of unsat clause indices
    std::vector<double> clause_unsat_idx_dist; // weight of unsat clauses 
    std::vector<weight_t> var_flip_weight_change; // gain this amount of weight if var[i] is flipped
};

// init_solution - 
void init_solution(const problem& problem, solution& solution, rand_t rand) {
    solution.assignment.clear();
    solution.assignment.push_back(0);
    for (uint64_t i=0; i < problem.num_variables; i++) {
        if (rand() >= 0.5) {
            solution.assignment.push_back(+1);
        } else {
            solution.assignment.push_back(-1);
        }
    }
}

// eval_solution - given assignment fill in its values
void eval_solution(const problem& problem, solution& solution) {
    // reset
    solution.clause_unsat_idx_list.clear();
    solution.var_flip_weight_change.clear();
    solution.var_flip_weight_change.push_back(0);
    for (uint64_t i=0; i < problem.num_variables; i++) {
        solution.var_flip_weight_change.push_back(0);
    }
    solution.assignment_weight = 0;
    // eval
    for (uint64_t i=0; i < problem.clause_list.size(); i++) {
        weight_t weight = problem.weight_list[i];
        const clause_t& clause = problem.clause_list[i];
        // process
        bool sat = false;
        var_t last_sat_var;
        uint64_t sat_var_count = 0;
        for (uint64_t j=0; j < clause.size(); j++) {
            lit_t lit = clause[j];
            var_t var = abs(lit);
            val_t val = solution.assignment[var];
            if (lit * val > 0) { // sat
                sat = true;
                last_sat_var = var;
                sat_var_count += 1;
                if (sat_var_count == 2) {
                    break;
                }
            }
        }
        // post process
        if (sat and sat_var_count == 1) {
            // last_sat_var makes the clause sat and if it is flipped, clause becomes unsat
            solution.var_flip_weight_change[last_sat_var] += weight;
        }
        if (not sat) {
            // every var makes the clause sat if it is flipped
            for (uint64_t j=0; j < clause.size(); j++) {
                lit_t lit = clause[j];
                var_t var = abs(lit);
                solution.var_flip_weight_change[var] -= weight;
            }
            // add to list of unsat clause
            solution.clause_unsat_idx_list.push_back(i);
            // update weight
            solution.assignment_weight += weight;
        }
    }
}

void make_clause_unsat_dist(const problem& problem, solution& solution) {
    // reset 
    solution.clause_unsat_idx_dist.clear();
    // set
    for (uint64_t i=0; i < solution.clause_unsat_idx_list.size(); i++) {
        uint64_t c = solution.clause_unsat_idx_list[i];
        solution.clause_unsat_idx_dist.push_back(problem.weight_list[c]);
    }
}

solution local_search_problem(const problem& problem, uint64_t seed, uint64_t max_time_s, double random_flip_prob, double reset_prob) {

    solution solution;

    std::uniform_real_distribution<double> dist_float01(0, 1);
    std::default_random_engine engine(seed);

    rand_t rand = [&engine, &dist_float01]() -> double {
        return dist_float01(engine);
    };

    init_solution(problem, solution, rand);
    uint64_t start_time_s = std::time(nullptr);
    uint64_t loop_count = 0;

    uint64_t best_assignment_weight = std::numeric_limits<uint64_t>::max();
    assign_t best_assignment(problem.num_variables+1);

    while (true) {
        loop_count++;

        eval_solution(problem, solution);
        if (solution.assignment_weight == 0.0) {
            return solution;
        }

        if (solution.assignment_weight < best_assignment_weight) {
            best_assignment_weight = solution.assignment_weight;
            copy_vector(best_assignment, solution.assignment);
        }

        uint64_t time_s = std::time(nullptr);
        if (time_s > start_time_s + max_time_s) {
            std::cout << "timeout: loop_count " << loop_count << std::endl;
            solution.assignment_weight = best_assignment_weight;
            copy_vector(solution.assignment, best_assignment);
            return solution;
        }

        if (rand() < reset_prob) { // reset and search again
            init_solution(problem, solution, rand);
            continue;
        }

        // flip
        // pick random unsat clause according to weight
        make_clause_unsat_dist(problem, solution);
        uint64_t i = weighted_random(solution.clause_unsat_idx_dist, rand());
        uint64_t c = solution.clause_unsat_idx_list[i];
        const clause_t& clause = problem.clause_list[c];
        var_t flip_var;
        if (rand() < random_flip_prob) {
            // with random_flip_prob, pick random var uniformly in clause
            flip_var = abs(clause[uint64_t(rand() * clause.size())]);
        } else {
            // pick the var with most weight change
            weight_t best_weight_change = std::numeric_limits<double>::max();
            for (lit_t lit : clause) {
                var_t var = abs(lit);
                if (solution.var_flip_weight_change[var] < best_weight_change) {
                    best_weight_change = solution.var_flip_weight_change[var];
                    flip_var = var;
                }
            }
        }
        // flip and repeat
        solution.assignment[flip_var] *= -1;
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
    // make problem
    problem problem;
    problem.num_variables = num_variables;
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
            problem.clause_list.push_back(clause);
        }
    }

    for (uint64_t c=0; c < num_clauses; c++) {
        problem.weight_list.push_back(clause_weight[c]);
    }

    solution solution = local_search_problem(problem, seed, max_time_s, rand_var_prob, 0.0);

    for (uint64_t v=0; v < num_variables+1; v++) {
        assignment[v] = solution.assignment[v];
    }

    return solution.assignment_weight;
}