from typing import Iterator
from walksat import walksat
from mpi_runner import run_task, Task, MPI_Comm

formula_dimacs = """
c This Formular is generated by mcnf
c
c    horn? no 
c    forced? no 
c    mixed sat? no 
c    clause length = 3 
c
p cnf 20  91 
 10 9 -6 0
8 19 10 0
1 18 13 0
-20 18 8 0
12 18 15 0
19 -14 -5 0
-3 -2 -17 0
10 -9 18 0
-4 -17 3 0
-20 -5 -3 0
17 -13 -4 0
14 -1 19 0
7 3 -18 0
-9 -8 12 0
-17 -19 10 0
-10 -11 -17 0
10 -5 15 0
-17 2 -10 0
-9 14 -1 0
-4 -1 -19 0
-17 9 -3 0
8 -11 -19 0
-9 14 -15 0
-11 -16 -12 0
11 -19 -17 0
2 9 5 0
-18 -9 -17 0
9 -19 -8 0
-2 -6 10 0
10 -8 -2 0
2 19 -6 0
-20 12 8 0
4 -1 9 0
9 -2 -14 0
-11 7 -20 0
17 3 -14 0
15 -5 -9 0
-4 12 -13 0
19 9 10 0
11 -13 -6 0
4 20 12 0
20 9 18 0
-6 12 20 0
14 1 15 0
-9 -12 -15 0
12 1 -11 0
12 6 -3 0
-18 13 3 0
4 6 -19 0
-14 18 -8 0
16 15 -2 0
-8 12 -2 0
-8 5 -16 0
1 15 -18 0
20 -19 -11 0
-6 -10 8 0
-18 -11 2 0
-1 18 -17 0
19 2 13 0
-5 -2 -20 0
-18 5 6 0
13 15 8 0
6 -5 -4 0
3 -14 12 0
11 -18 -1 0
1 -19 20 0
-11 -7 18 0
6 18 9 0
-9 -4 14 0
1 7 -2 0
4 -19 -17 0
-20 -19 18 0
-2 8 -11 0
-19 13 -4 0
20 4 1 0
-18 -11 -20 0
-15 -19 -10 0
14 17 -8 0
-15 7 -9 0
-12 5 15 0
-12 -3 -13 0
-2 12 11 0
-11 -16 3 0
-14 17 -3 0
-4 -6 8 0
-7 -14 -2 0
-19 2 4 0
-1 -20 11 0
-12 13 -18 0
8 -4 16 0
-9 6 19 0
c adding one clause
1 0
"""

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
        self.size = comm.get_size()
        self.formula = list(parse_dimacs(formula_dimacs=formula_dimacs))[0]
    
    def produce(self):
        for i in range(1 * (self.size - 1)): # 1 job for each worker
            yield self.formula
    
    def consume(self, result):
        best_num_unsat_clauses, assign = result
        print(best_num_unsat_clauses, assign)

    def setup_worker(self, comm = None):
        self.seed = comm.get_rank() + 1000
        self.step = comm.get_size() - 1

    def apply(self, item):
        formula = item
        print(f"running walksat with seed {self.seed}")
        best_num_unsat_clauses, assignment = walksat(
            formula=formula,
            seed=self.seed,
            max_time_s=5,
            rand_var_prob=0.3,
        )
        self.seed += self.step
        return best_num_unsat_clauses, assignment

if __name__ == "__main__":
    from mpi4py import MPI
    run_task(task=MyTask(), comm=MPI_Comm(MPI=MPI))