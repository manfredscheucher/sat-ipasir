# sat-examples

A small collection of SAT modelling examples using **ipasirup** with both

* **Python (PySAT)** and
* **C++ (IPASIR + CaDiCaL/Glucose)**

The goal is to provide minimal, reproducible *model enumeration* building blocks.


---

## Quick start

### Python

Follow the official [PySAT installation guide](https://pysat.readthedocs.io/en/latest/installation.html). Note that while they recommend **conda**, using Python’s built-in **venv** also works fine:

```bash
python -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install python-sat[pblib,aiger]
```

### C++ (IPASIR with CaDiCaL)

Follow the upstream build instructions from [https://github.com/arminbiere/cadical](https://github.com/arminbiere/cadical) (see `BUILD.md`).

Minimal build (after building CaDiCaL once):

```bash
export CADICAL=$HOME/github/cadical   # path to your CaDiCaL checkout
# expects $CADICAL/build/libcadical.a and $CADICAL/build/ipasir.o after `./configure && make`
```

---

## PySAT

If you are new to PySAT or want a quick refresher:

- 📺 Talk (YouTube): [https://www.youtube.com/watch?v=_c9bVMkFqYw](https://www.youtube.com/watch?v=_c9bVMkFqYw)
- 🧪 Interactive Colab: [https://colab.research.google.com/drive/1qycPKqLiCqDC5mt5HS89npaat3bxm5bR](https://colab.research.google.com/drive/1qycPKqLiCqDC5mt5HS89npaat3bxm5bR)

**Contents in `examples/pysat/`:**

- ✎ `PHDSCHOOL-NOTEBOOK.ipynb` — notebook from phdschool (same as in colab)
- ✎ `example1.py ` — example 1: tiny example with pysat
- ✎ `example2_ramsey.py ` — example 2: ramsey numbers
- ✎ `cardinality-encodings.py ` — cardinality encodings
- ✎ `dpll.py ` — performance-comparison dpll and bruteforce (cadical implements CDCL)
- ✎ `exercise1_sudoku.py ` — solution to exercise 1: sudoku completion
- ✎ `example3_erdos_szekeres.py ` — example 3: Erdös-Szekeres numbers

	
				

---

## IPASIR-UP Examples

### 1) `ipasirup-ksubsets` : Enumerating k-subsets of {1..n} 

We use `n` indicator variables (x_1, …, x_n). Both approaches return (\binom{n}{k}):

* **`ksubset1` (look-ahead):** as soon as the partial assignment reaches exactly `k` true variables, add a clause that forbids setting any additional variable to `true` (prunes the branch early). Analogously, if exactly `n-k`false variables are reached, a clause is added that forbids setting any additional variable to `false`.
* **`ksubset2` (conflict-driven):** when a conflict is reached (i.e., going beyond `k` true variables or beyond `n-k`false variables) add a clause to cut the branch. 


### 2) `ipasirup-graphs` : Enumerating unlabeled graphs on n vertices

We use an n-times-n adjacency matrix of indicator variables. The encoding follows the same conflict-driven idea as `ksubset2`, but with **lexicographic minimality** as a symmetry-breaking constraint. As soon as the partial assignment cannot remain lex-min, the branch is cut. The resulting counts match the number of non-isomorphic unlabeled n-vertex graphs [OEIS/A88](https://oeis.org/A88). 
