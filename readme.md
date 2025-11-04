# sat-examples

A small collection of SAT modelling examples using **ipasirup** with both

* **Python (PySAT)** and
* **C++ (IPASIR + CaDiCaL/Glucose)**

The goal is to provide minimal, reproducible *model enumeration* building blocks.

## Repository layout

This README keeps your existing repository structure. The examples live under `examples/`.

```
./
├── LICENSE
└── examples/
    ├── ... (k-subset example files)
    └── ... (graph enumeration example files)
```

> Each example directory contains its **own `Makefile`** (templates below). Adjust only file names if needed—no restructuring required.

File names here are placeholders—adjust to your actual files in the repo.

---

## Quick start

### Python

**Requirements**

* Python 3.9+
* [PySAT](https://pysat.readthedocs.io/en/latest/installation.html)

**Install**
You can follow the official PySAT installation guide (they show **conda**). Using Python’s built-in **venv** also works fine:

```bash
python -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install python-sat[pblib,aiger]
```

### C++ (IPASIR with CaDiCaL)

Follow the upstream build instructions:

* [https://github.com/arminbiere/cadical](https://github.com/arminbiere/cadical) (see `BUILD.md`)

Minimal build (after building CaDiCaL once):

```bash
export CADICAL=$HOME/github/cadical   # path to your CaDiCaL checkout
# expects $CADICAL/build/libcadical.a and $CADICAL/build/ipasir.o after `./configure && make`
```

---

## What the examples do

### 1) Enumerating k-subsets of {1..n}

We use `n` indicator variables (x_1, …, x_n). Both approaches return (\binom{n}{k}):

* **`ksubset1` (look-ahead):** as soon as the partial assignment reaches exactly `k` true variables, add a clause that forbids setting any additional variable to `true` (prunes the branch early). Analogously, if exactly `n-k`false variables are reached, a clause is added that forbids setting any additional variable to `false`.
* **`ksubset2` (conflict-driven):** when a conflict is reached (i.e., going beyond `k` true variables or beyond `n-k`false variables) add a clause to cut the branch. 


### 2) Enumerating unlabeled graphs on n vertices

We use an n-times-n adjacency matrix of indicator variables. The encoding follows the same conflict-driven idea as `ksubset2`, but with **lexicographic minimality** as a symmetry-breaking constraint. As soon as the partial assignment cannot remain lex-min, the branch is cut. The resulting counts match the number of non-isomorphic unlabeled n-vertex graphs [OEIS/A88](https://oeis.org/A88). 
