from pysat.card import CardEnc
print(CardEnc.atmost(lits=[1,2,3,4,5,6,7,8],bound=7).clauses)

# at most 500 among 1000
print(len(list(CardEnc.atmost(lits=range(1,1000),bound=500).clauses)))