
from itertools import combinations
from pysat.formula import IDPool
from pysat.solvers import Cadical195

def erdos_szekeres_instance(k,n,break_symmetries=True):
	N = range(n)
	vpool = IDPool()

	# declare variables in dictionary
	var_trip_ = {I: vpool.id() for I in combinations(N,3)}
	var_conv_ = {I: vpool.id() for I in combinations(N,4)}

	# access variables with () instead [] for syntax highlighting
	def var_trip(*I): return var_trip_[I]
	def var_conv(*I): return var_conv_[I]

	cnf = []

	# forbid invalid configuartions in the signature
	for I4 in combinations(N,4):
		I4_triples = list(combinations(I4,3))
		for t1,t2,t3 in combinations(I4_triples,3):
			# for any three lexicographical ordered triples t1 < t2 < t3
			# the signature must not be "+-+" or "-+-"
			cnf.append([+var_trip(*t1),-var_trip(*t2),+var_trip(*t3)])
			cnf.append([-var_trip(*t1),+var_trip(*t2),-var_trip(*t3)])

	if break_symmetries:
		# without loss of generality, points sorted around 0
	  for b,c in combinations(range(1,n),2):
		  cnf.append([ var_trip(0,b,c)])

	# assert crossings (8 patterns on 4 points, 4 of them are convex)
	for I in combinations(N,4):
		[a,b,c,d] = I
		# if not convex position, ab or cd is not extremal
		cnf.append([ var_conv(*I),  var_trip(a,b,c),  var_trip(b,c,d)])
		cnf.append([ var_conv(*I),  var_trip(a,b,d), -var_trip(a,c,d)])
		cnf.append([ var_conv(*I), -var_trip(a,b,d),  var_trip(a,c,d)])
		cnf.append([ var_conv(*I), -var_trip(a,b,c), -var_trip(b,c,d)])

	# assert no k-gons via caratheodory (at least one non-convex 4-tuple)
	for I in combinations(N,k):
		cnf.append([-var_conv(*J) for J in combinations(I,4)])

	return cnf

cnf58 = erdos_szekeres_instance(5,8)
solver = Cadical195(bootstrap_with=cnf58)
print("ES(5)>8:",solver.solve())

cnf59 = erdos_szekeres_instance(5,9)
solver = Cadical195(bootstrap_with=cnf59)
print("ES(5)>9:",solver.solve())

cnf616 = erdos_szekeres_instance(6,16)
solver = Cadical195(bootstrap_with=cnf616)
print("ES(6)>16:",solver.solve())

cnf617 = erdos_szekeres_instance(6,17)
solver = Cadical195(bootstrap_with=cnf617)
print("(the following computation will take about 15 minutes)")
print("ES(6)>17:",solver.solve()) 
