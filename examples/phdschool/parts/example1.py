from pysat.solvers import Cadical195

cnf = [[1,2,3],[-1,-2,-3]]
solver = Cadical195(bootstrap_with=cnf)
for sol in solver.enum_models(): print(sol)
