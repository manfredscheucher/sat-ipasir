from pysat.solvers import Cadical195
from pysat.formula import IDPool
from itertools import combinations, permutations
from itertools import combinations

# creates a CNF instance for deciding whether
# the ramsey number R(k,l) is at least n
def ramsey_instance(n,a,b):
	vertices = range(n)
	vpool = IDPool()
	edge_var = {(u,v):vpool.id() for (u,v) in combinations(vertices,2)}
	cnf = []
	for I in combinations(vertices,a):
		cnf.append([-edge_var[u,v] for u,v in combinations(I,2)])
	for I in combinations(vertices,b):
		cnf.append([+edge_var[u,v] for u,v in combinations(I,2)])
	return cnf,edge_var


if __name__ == '__main__':
	from timed import timed

	# compute R(3,3)
	with timed():
		cnf533,edge_var = ramsey_instance(5,3,3)
		print("cnf",cnf533)
		print("edge_var",edge_var)
		solver = Cadical195(bootstrap_with=cnf533)
		print("R(3,3)>5:",solver.solve())
		for sol in solver.enum_models():
		  edges = [e for e in edge_var if edge_var[e] in sol]
		  print(sol,"->",edges)

	with timed():
		cnf633,edge_var = ramsey_instance(6,3,3)
		solver = Cadical195(bootstrap_with=cnf633)
		print("R(3,3)>6:",solver.solve())

	# compute R(3,4)
	with timed():
		cnf834,edge_var = ramsey_instance(8,3,4)
		solver = Cadical195(bootstrap_with=cnf834)
		print("R(3,4)>8:",solver.solve())

	with timed():
		# compute R(3,4)
		cnf934,edge_var = ramsey_instance(9,3,4)
		solver = Cadical195(bootstrap_with=cnf934)
		print("R(3,4)>9:",solver.solve())

	# compute R(3,5)
	with timed():
		cnf1335,edge_var = ramsey_instance(13,3,5)
		solver = Cadical195(bootstrap_with=cnf1335)
		print("R(3,5)>13:",solver.solve())

	with timed():
		cnf1435,edge_var = ramsey_instance(14,3,5)
		solver = Cadical195(bootstrap_with=cnf1435)
		print("the following command will take a while...")
		print("R(3,5)>14:",solver.solve())

