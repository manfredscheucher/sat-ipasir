from pysat.solvers import Cadical195
from pysat.formula import IDPool
from itertools import combinations
#from itertools import batched  # available for newer python versions, colab is only 3.10
from more_itertools import chunked
def batched(L,r): return list(chunked(L,r))

assignment = {
(1,2):3,
(2,4):1,
(2,5):9,
(2,6):5,
(3,3):8,
(3,8):6,
(4,1):8,
(4,5):6,
(5,1):4,
(5,4):8,
(5,9):1,
(6,5):2,
(7,2):6,
(7,7):2,
(7,8):8,
(8,4):4,
(8,5):1,
(8,6):9,
(8,9):5,
(9,8):7,
}


values = range(1,10) # [1,2,3,4,5,6,7,8,9]

# create dictionary for variables (map anything to 1,2,3,...)
vpool = IDPool()
# variables to indicate whether in row r, column c, the value is v
X = {(r,c,v):vpool.id() for r in values for c in values for v in values}

# Note: one can also use "X = lambda(r,c,v): vpool.id(r,c,v)"
# this allows to write X(r,c,v) (function call) instead of X[r,c,v] (dictionary)

cnf = []

# each field has one value
for r in values:
	for c in values:
		cnf.append([X[r,c,v] for v in values])
		for v1,v2 in combinations(values,2):
			cnf.append([-X[r,c,v1],-X[r,c,v2]])

# every value appears one per row
for r in values:
	for v in values:
		cnf.append([X[r,c,v] for c in values])

# every value appears one per column
for c in values:
	for v in values:
		cnf.append([X[r,c,v] for r in values])

# every value appears one per box
value_blocks = batched(values,3) # [1,...9] -> [1,2,3],[4,5,6],[7,8,9]
for v in values:
	for r_block in value_blocks:
		for c_block in value_blocks:
			cnf.append([X[r,c,v] for r in r_block for c in c_block])

# fix prescribed entries
for r,c in assignment:
	v = assignment[r,c]
	cnf.append([X[r,c,v]])

solver = Cadical195(bootstrap_with=cnf)

#ct=0
#for solution in solver.enum_models():
#	ct += 1
for ct,solution in enumerate(solver.enum_models()):
	print(f"solution #{ct+1}")
	for r in values:
		for c in values:
			for v in values:
				if X[r,c,v] in solution:
					print(f"{v}*" if (r,c) in assignment else f"{v} ",end=" ")
		print()
	if ct >= 2: break # stop after 3 solutions