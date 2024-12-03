from walksat import c_walksat

satisfiable, assignment = c_walksat(formula=[[1, -2], [2, 3]])
print(satisfiable, assignment)
satisfiable, assignment = c_walksat(formula=[[1, -2], [-1], [2]])
print(satisfiable, assignment)