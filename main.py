from walksat import c_walksat

satisfiable, assignment = c_walksat(max_time_s=10, formula=[[-6, -2], [-2, 3, +3]])
print(satisfiable, assignment)