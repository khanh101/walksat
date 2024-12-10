from typing import Iterator
from walksat import walksat
import random
from mpi_runner import run_task, Task, MPI_Comm

num_variables = 2
formula_dimacs = """
p cnf 2 4 
1 2 0
1 0
-2 0
-1 0
"""

weight = [1, 1, 0, 1]

def parse_dimacs(formula_dimacs: str) -> Iterator[list[list[int]]]:
    problem = None
    for line in formula_dimacs.split("\n"):
        line = line.strip()
        if len(line) == 0:
            continue
        splits = line.split()
        if splits[0] == "c":
            continue
    
        if splits[0] == "p": # new problem
            if problem is not None:
                yield [c for c in problem if len(c) > 0]
            
            problem = [[]]
            continue
        
        splits_int = [int(s) for s in splits]
        for i in splits_int:
            if i != 0:
                problem[-1].append(i)
            else:
                problem.append([])

    
    if problem is not None:
        yield [c for c in problem if len(c) > 0]

class MyTask(Task):
    def setup(self, comm = None):
        self.num_workers = 1
        if comm.get_size() > 1:
            self.num_workers = comm.get_size() - 1

        self.formula = list(parse_dimacs(formula_dimacs=formula_dimacs))[0]
        self.weight = weight
    
    def produce(self):
        for i in range(4 * self.num_workers): # 4 job for each worker
            yield self.formula, self.weight
    
    def consume(self, result):
        seed, best_num_unsat_clauses, assignment = result
        print(seed, best_num_unsat_clauses, assignment)

    def setup_worker(self, comm = None):
        random.seed(comm.get_rank() + 1234)

    def apply(self, item):
        formula, weight = item
        seed = random.randrange(2**64)
        best_num_unsat_clauses, assignment = walksat(
            num_variables=num_variables,
            formula=formula,
            weight=weight,
            seed=seed,
            max_time_s=2,
            rand_var_prob=0.2,
        )
        return seed, best_num_unsat_clauses, assignment

if __name__ == "__main__":
    from mpi4py import MPI
    run_task(task=MyTask(), comm=MPI_Comm(MPI=MPI))