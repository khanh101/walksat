from walksat import c_walksat

satisfiable, assignment = c_walksat(seed=12342, max_time_s=10, formula=[[1, -2], [2, 3]])
print(satisfiable, assignment)
satisfiable, assignment = c_walksat(seed=12342, max_time_s=10, formula=[[1, -2], [-1], [2]])
print(satisfiable, assignment)