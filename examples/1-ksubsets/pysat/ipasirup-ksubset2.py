from pysat.formula import CNF
from pysat.solvers import Solver
from pysat.engines import Propagator
from itertools import combinations
from math import comb
from sys import argv

n = int(argv[1])
k = int(argv[2])
assert 0 <= k <= n

CHECK_AT_LEAST_CONDITION = True
CHECK_AT_MOST_CONDITION = True

vars_ids = set(range(1,n+1))
cnf = CNF()

check_model_calls = 0
propagate_calls = 0

class ChooseK(Propagator):
    def __init__(self, n, k):
        super().__init__()
        self.n = n
        self.k = k
        self.level = 0

        #self.assigned = set()         # set of all assigned literals
        self.assigned_at_level = [set()]     # for each level: list of literals assigned 
        self.assigned_positive = set()
        self.assigned_negative = set()

        self.reason = {}                  # reason for literals
        self.reason_at_level = [set()]    # per level
        self.queue = []                   # literals queued for propagation 
        self.pending = []                # clause queued to be added, from check_model()

    def setup_observe(self, s):
        for v in vars_ids:
            s.observe(v)

    def on_new_level(self):
        self.level += 1
        self.assigned_at_level.append(set())
        self.reason_at_level.append(set())
        assert len(self.assigned_at_level) == self.level + 1
        assert len(self.reason_at_level) == self.level + 1

    def on_backtrack(self, to):
        while self.level > to:
            for lit in self.assigned_at_level[self.level]:
                #self.assigned.discard(lit)
                if lit > 0:
                    self.assigned_positive.discard(lit)
                else:
                    self.assigned_negative.discard(-lit)
            self.assigned_at_level.pop()

            for lit in self.reason_at_level[self.level]:
                self.reason.pop(lit, None)
            self.reason_at_level.pop()

            self.level -= 1
        assert(not self.queue)
        self.queue.clear()

    # check partial assignment
    def on_assignment(self, lit, fixed=False):
        assert lit not in self.assigned_positive and lit not in self.assigned_negative
        self.assigned_at_level[self.level].add(lit)
        if lit > 0:
            self.assigned_positive.add(lit)
        else:
            self.assigned_negative.add(-lit)

        if CHECK_AT_MOST_CONDITION and len(self.assigned_positive) > self.k:
            conflict_clause = [-t for t in sorted(self.assigned_positive)]
            impl = conflict_clause[0]
            self.queue.append(impl)
            self.reason[impl] = conflict_clause
            return

        if CHECK_AT_LEAST_CONDITION and len(self.assigned_negative) > self.n-self.k:
            conflict_clause = [+t for t in sorted(self.assigned_negative)]
            impl = conflict_clause[0]
            self.queue.append(impl)
            self.reason[impl] = conflict_clause
            return

    def propagate(self):
        if not self.queue:
            return []
        global propagate_calls
        propagate_calls += 1
        lits = self.queue
        self.queue = []
        return lits

    def provide_reason(self, lit):
        r = self.reason.pop(lit)
        for s in self.reason_at_level:
            s.discard(lit)
        return r

    # check exactly-k on full assignment
    def check_model(self, model):
        global check_model_calls
        check_model_calls += 1

        trues = {l for l in model if l > 0 and l <= self.n}
        if len(trues) != self.k:
            self.pending = [-l for l in model]
            return False
            
        return True

    def add_clause(self):
        cls = self.pending
        self.pending = []
        return cls


solutions = []
with Solver(name="cadical195", bootstrap_with=cnf.clauses) as s:
    p = ChooseK(n, k)
    s.connect_propagator(p)
    p.setup_observe(s)

    while s.solve():
        model = s.get_model()
        sol = [l for l in model if l > 0]
        solutions.append(sol)
        s.add_clause([-l for l in model])

print(f"{len(solutions)} solutions, expected: C({n},{k}) = {comb(n,k)}")
assert len(solutions) == comb(n,k)
# Optional: print(solutions)
print(f"check_model_calls: {check_model_calls}")
print(f"propagate_calls: {propagate_calls}")
