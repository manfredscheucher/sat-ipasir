
# a bruteforce approach to decide satisfiability of CNF instances
def bruteforce(cnf):
  if cnf == []:
    return True # no clauses = SAT
  for clause in cnf:
    if clause == []:
      return False # emtpy clause = UNSAT
  v = choice(variables(cnf))
  cnf1 = cnf_force_literal(cnf,+v)
  cnf2 = cnf_force_literal(cnf,-v)
  return bruteforce(cnf1) or bruteforce(cnf2)

def variables(cnf):
  return {abs(lit) for lit in literals(cnf)}

def literals(cnf):
  lits = set()
  for clause in cnf:
    for lit in clause:
      lits.add(lit)
  return lits

def cnf_force_literal(cnf,lit):
  cnf_new = []
  for clause in cnf:
    if lit in clause:
      pass # a clause containing lit is fulfilled and can be omited
    else:
      # otherwise omit negated occurences of lit
      cnf_new.append([l for l in clause if l != -lit])
  return cnf_new

def choice(v):
  return min(v) # fix some selection strategy

def dpll(cnf):
  cnf = unit_propagation(cnf)
  # cnf = pure_literal_rule(cnf) # optional
  # rest as in naive algorithm
  if cnf == []:
    return True # no clauses = SAT
  for clause in cnf:
    if clause == []:
      return False # emtpy clause = UNSAT
  v = choice(variables(cnf))
  cnf1 = cnf_force_literal(cnf,+v)
  cnf2 = cnf_force_literal(cnf,-v)
  return dpll(cnf1) or dpll(cnf2)

def unit_propagation(cnf):
  while True:
    changes = False
    for clause in cnf:
      if len(clause) == 1:
        lit = clause[0]
        cnf = cnf_force_literal(cnf,lit)
        changes = True
    if not changes:
      return cnf # stop if no more changes

#def pure_literal_rule(cnf):
#  lits = literals(cnf)
#  for lit in lits:
#    if -lit not in lits:
#      cnf = cnf_force_literal(cnf,l)
#  return cnf

if __name__ == '__main__':
	from timed import timed
	from example2_ramsey import ramsey_instance
	cnf834,edge_var = ramsey_instance(8,3,4)
	with timed("bruteforce"):
		print("R(3,4)>8:",bruteforce(cnf834))
	with timed("dpll"):
		print("R(3,4)>8:",dpll(cnf834))
