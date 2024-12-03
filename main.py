from walksat import c_walksat

satisfiable, assignment = c_walksat(seed=12342, max_time_s=10, formula=[[1, -2], [2, -1]])
print(satisfiable, assignment)