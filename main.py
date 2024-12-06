from typing import Iterator
from walksat import walksat

formula_dimacs = """
c sample.cnf
c
c This is an example of the CNF-SAT problem data
c in DIMACS format.
c
p cnf 4 3
1 2 0
-4 3
-2 0
-1 4 0
c
c eof
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
                if len(problem[-1]) == 0:
                    problem = problem[:-1]
                yield problem
            
            problem = [[]]
            continue
        
        splits_int = [int(s) for s in splits]
        for i in splits_int:
            if i != 0:
                problem[-1].append(i)
            else:
                problem.append([])

    
    if problem is not None:
        if len(problem[-1]) == 0:
            problem = problem[:-1]
        yield problem

formula = list(parse_dimacs(formula_dimacs=formula_dimacs))[0]

satisfiable, assignment = walksat(formula=formula)
print(satisfiable, assignment)
satisfiable, assignment = walksat(formula=[[1, -2], [-1], [2]])
print(satisfiable, assignment)