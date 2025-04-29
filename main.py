import sys
from typing import Iterator
from walksat import walksat
from mpi_runner import run_task, Task, MPI_Comm



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
    size: int
    formula: list[list[int]]
    weight: list[float]
    seed: int
    step: int
    def setup(self, comm = None):
        self.size = comm.get_size()
        self.formula = list(parse_dimacs(formula_dimacs=open(sys.argv[1]).read()))[0]
        self.weight = [1.0 for _ in range(len(self.formula))]
        self.weight[2] = 0
    
    def produce(self):
        for i in range(1 * (self.size - 1)): # 1 job for each worker
            yield self.formula, self.weight
    
    def consume(self, result):
        best_num_unsat_clauses, assign = result
        print(best_num_unsat_clauses)

    def setup_worker(self, comm = None):
        self.seed = comm.get_rank() + 1000
        self.step = comm.get_size() - 1

    def apply(self, item):
        formula, weight = item
        print(f"running walksat with seed {self.seed}")
        best_num_unsat_clauses, assignment = walksat(
            formula=formula,
            weight=weight,
            seed=self.seed,
            max_time_s=5,
            rand_var_prob=0.1,
        )
        self.seed += self.step
        return best_num_unsat_clauses, assignment

if __name__ == "__main__":
    from mpi4py import MPI
    run_task(task=MyTask(), comm=MPI_Comm(MPI=MPI))