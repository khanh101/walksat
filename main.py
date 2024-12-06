from walksat import walksat

satisfiable, assignment = walksat(formula=[[1, -2], [2, 3]])
print(satisfiable, assignment)
satisfiable, assignment = walksat(formula=[[1, -2], [-1], [2]])
print(satisfiable, assignment)