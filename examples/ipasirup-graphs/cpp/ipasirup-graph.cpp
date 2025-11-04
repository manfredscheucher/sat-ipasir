// Build:
//   CADICAL=~/github/cadical
//   clang++ -O2 -std=c++17 graph_symmetry_extprop.cpp \
//     -I"$CADICAL/src" "$CADICAL/build/libcadical.a" -o graphsym
//
// Run:
//   ./graphsym 5
//
// Idea: Variables are edges i<j of an undirected graph with n vertices.
//       The propagator enforces a canonical (colex-minimal) adjacency under permutations.
//       On violation it generates a unit implication with a reason clause,
//       mirroring your Python minCheck/on_assignment.

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cadical.hpp"  // -I<cadical>/src

using std::pair;
using std::vector;
using std::unordered_map;
using std::unordered_set;

// ---------- Combinatorics / Helpers ----------

// Edge IDs: map (i<j) -> var_id in [1..m], m = nC2; and back.
struct EdgeMap {
    int n;
    vector<pair<int,int>> id2edge;          // 1-based index -> (i,j)
    unordered_map<long long,int> edge2id;   // key(i,j) -> id

    static long long key(int i, int j) { return ( (long long)i<<32 ) ^ (long long)j; }

    explicit EdgeMap(int n_) : n(n_) {
        int id = 1;
        id2edge.resize(n*(n-1)/2 + 1); // 1-based
        for (int i = 0; i < n; ++i) for (int j = i+1; j < n; ++j) {
            id2edge[id] = {i,j};
            edge2id[key(i,j)] = id;
            edge2id[key(j,i)] = id; // undirected: symmetric
            ++id;
        }
    }
    inline int id_of(int i, int j) const {
        auto it = edge2id.find(key(i,j));
        return (it==edge2id.end()) ? 0 : it->second;
    }
};

// Colex order for 2-combinations of {0..k-1}:
// Result is ascending by (j then i): (0,1),(0,2),(1,2),(0,3),(1,3),(2,3),…
static vector<pair<int,int>> colex_pairs(int k) {
    vector<pair<int,int>> out;
    for (int j = 1; j < k; ++j)
        for (int i = 0; i < j; ++i)
            out.push_back({i,j});
    return out;
}

// Permute A according to 'perm' (submatrix perm×perm only).
// A[r][c]: -1 = unset, 0 = false, 1 = true
static vector<vector<int>> permute_sub(const vector<vector<int>>& A, const vector<int>& perm) {
    int k = (int)perm.size();
    vector<vector<int>> B(k, vector<int>(k, 0));
    for (int rr = 0; rr < k; ++rr)
        for (int cc = 0; cc < k; ++cc)
            B[rr][cc] = A[perm[rr]][perm[cc]];
    return B;
}

// Fingerprint in colex order over a k×k matrix (upper triangle)
static vector<int> fingerprint_colex(const vector<vector<int>>& M) {
    int k = (int)M.size();
    vector<int> fp;
    fp.reserve(k*(k-1)/2);
    auto ord = colex_pairs(k);
    for (auto [i,j] : ord) fp.push_back(M[i][j]);
    return fp;
}

// Replace -1 (unset) by newvalue (0/1)
static vector<int> replace_unset(const vector<int>& L, int newvalue) {
    vector<int> out = L;
    for (int& x : out) if (x < 0) x = newvalue;
    return out;
}

// ---------- Propagator ----------

struct GraphProp : public CaDiCaL::ExternalPropagator {
    CaDiCaL::Solver* S = nullptr;
    const int n;
    const EdgeMap edges;

    // Assignment tracking
    size_t level = 0;
    vector<unordered_set<int>> assigned_lits_at_level{1};
    vector<unordered_set<int>> reason_lits_at_level{1};
    unordered_set<int> pos_ids; // {edge-id} set to TRUE
    unordered_set<int> neg_ids; // {edge-id} set to FALSE

    // Propagation queue
    std::queue<int> prop_queue;
    unordered_map<int, vector<int>> reason_of; // implied lit -> reason clause

    // Stats
    unsigned long long propagate_calls = 0;

    explicit GraphProp(int n_) : n(n_), edges(n_) {}

    void init(CaDiCaL::Solver* s) { S = s; }

    // Build current n×n adjacency matrix from partial assignment
    vector<vector<int>> adj_matrix() const {
        vector<vector<int>> A(n, vector<int>(n, 0));
        for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) A[i][j] = (i==j)?0:-1;
        int m = n*(n-1)/2;
        for (int id = 1; id <= m; ++id) {
            auto [r,c] = edges.id2edge[id];
            if (pos_ids.count(id)) A[r][c] = A[c][r] = 1;
            else if (neg_ids.count(id)) A[r][c] = A[c][r] = 0;
            else A[r][c] = A[c][r] = -1;
        }
        return A;
    }

    // Python: combinations_colex_ordered(range(k),2) on the base side and on the permuted side.
    // Returns, at the first “B_fp < A_fp” witness, a conflict clause.
    // conflict_clause = [+x for x in must_be_positive] + [-x for x in must_be_negative]
    // Return empty => no cut (not yet decidable).
    vector<int> first_conflict_clause() {
        auto A = adj_matrix();
        // recursive DFS over permutations:
        vector<int> perm; perm.reserve(n);
        vector<char> used(n, 0);
        vector<int> clause;

        // colex descriptor for the base (built dynamically per k)
        std::function<bool()> dfs = [&]() -> bool {
            int k = (int)perm.size();
            // Submatrices A_ and B_
            vector<int> base_idx(k); // 0..k-1 (identity)
            for (int i = 0; i < k; ++i) base_idx[i] = i;

            auto A_ = permute_sub(A, base_idx);
            auto B_ = permute_sub(A, perm);

            auto A_fp = fingerprint_colex(A_);
            auto B_fp = fingerprint_colex(B_);
            // unset: A best (=0), B worst (=1)
            A_fp = replace_unset(A_fp, 0);
            B_fp = replace_unset(B_fp, 1);

            // Prune if B>A (colex)
            if (B_fp > A_fp) return false;

            if (k == n) {
                // Full permutation
                if (B_fp < A_fp) {
                    // Ordered pairs in colex for base and permuted indices
                    auto ordA = colex_pairs(k);          // pairs over 0..k-1
                    vector<pair<int,int>> ordB; ordB.reserve(ordA.size());
                    for (auto [i,j] : ordA) ordB.push_back({perm[i], perm[j]});

                    // Build clause
                    unordered_set<int> must_pos, must_neg;
                    bool found=false;
                    for (size_t l = 0; l < B_fp.size(); ++l) {
                        if (B_fp[l] == 0) {
                            int id = edges.id_of(ordB[l].first, ordB[l].second);
                            must_pos.insert(id);
                        }
                        if (A_fp[l] == 1) {
                            int id = edges.id_of(ordA[l].first, ordA[l].second);
                            must_neg.insert(id);
                        }
                        if (B_fp[l] != A_fp[l]) {
                            // first difference must be (B=0, A=1)
                            // conflict_clause = +must_pos ∪ −must_neg
                            clause.clear();
                            clause.reserve(must_pos.size() + must_neg.size());
                            for (int x : must_pos) clause.push_back(+x);
                            for (int x : must_neg) clause.push_back(-x);
                            found = true;
                            break;
                        }
                    }
                    assert(found);
                    return true; // clause set
                }
            } else {
                // expand
                for (int v = 0; v < n; ++v) if (!used[v]) {
                    used[v] = 1;
                    perm.push_back(v);
                    if (dfs()) return true; // propagate found
                    perm.pop_back();
                    used[v] = 0;
                }
            }
            return false;
        };

        if (dfs()) return clause;
        return {}; // no conflict derivable
    }

    // === ExternalPropagator Interface ===

    void notify_new_decision_level() override {
        ++level;
        assigned_lits_at_level.emplace_back();
        reason_lits_at_level.emplace_back();
    }

    void notify_backtrack(size_t new_level) override {
        while (level > new_level) {
            for (int lit : assigned_lits_at_level[level]) {
                if (lit > 0) pos_ids.erase(lit);
                else          neg_ids.erase(-lit);
            }
            assigned_lits_at_level.pop_back();
            for (int lit : reason_lits_at_level[level]) reason_of.erase(lit);
            reason_lits_at_level.pop_back();
            --level;
        }
        // discard scheduled propagation
        while (!prop_queue.empty()) prop_queue.pop();
    }

    // Batch of trail assignments
    void notify_assignment(const vector<int>& lits) override {
        for (int lit : lits) {
            assigned_lits_at_level[level].insert(lit);
            if (lit > 0) pos_ids.insert(lit);
            else          neg_ids.insert(-lit);
        }

        // Try to generate a symmetry cut as a unit implication
        auto cls = first_conflict_clause();
        if (!cls.empty()) {
            // As in your Python: impl = conflict_clause[0]
            // Enqueue that implied literal with the full reason.
            int impl = cls.front();
            if (!reason_of.count(impl)) {
                prop_queue.push(impl);
                reason_of[impl] = cls;                 // full reason clause
                reason_lits_at_level[level].insert(impl);
            }
        }
    }

    // Return one literal to propagate (0 = none)
    int cb_propagate() override {
        if (prop_queue.empty()) return 0;
        ++propagate_calls;
        int lit = prop_queue.front(); prop_queue.pop();
        return lit;
    }

    // Stream the reason clause for 'propagated_lit' — literals one by one; 0 ends.
    int cb_add_reason_clause_lit(int propagated_lit) override {
        auto it = reason_of.find(propagated_lit);
        if (it == reason_of.end()) return 0;
        auto &cls = it->second;
        if (cls.empty()) {
            reason_of.erase(it);
            return 0;
        }
        int lit = cls.back();
        cls.pop_back();
        return lit;
    }

    int cb_decide() override { return 0; }

    bool cb_check_found_model(const vector<int>& /*model*/) override {
        return true;
    }

    // External clauses (not used here, but required by the interface)
    bool cb_has_external_clause(bool &is_forgettable) override {
        is_forgettable = true;     // may be forgotten by the solver
        return false;              // we don't have an external clause to add right now
    }

    int cb_add_external_clause_lit() override {
        return 0;                  // 0 terminates (no literals to add)
    }
};

// OEIS A000088 (number of unlabeled graphs), for small n as a sanity check.
static const unsigned long long expected_count[] = {
    1ULL, 1ULL, 2ULL, 4ULL, 11ULL, 34ULL, 156ULL, 1044ULL, 12346ULL,
    274668ULL, 12005168ULL, 1018997864ULL, 165091172592ULL
};

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: " << argv[0] << " n\n"; return 1; }
    int n = std::atoi(argv[1]);
    if (n < 0) { std::cerr << "n>=0 please\n"; return 1; }

    // Number of variables = edges = nC2; we number 1..m
    int m = n * (n - 1) / 2;

    CaDiCaL::Solver S;
    GraphProp P(n);
    S.connect_external_propagator(&P);
    P.init(&S);

    // Register observed variables, otherwise no callbacks:
    for (int v = 1; v <= m; ++v) S.add_observed_var(v);

    unsigned long long solutions = 0;

    while (true) {
        int code = S.solve();         // 10 SAT, 20 UNSAT
        if (code == 20) break;
        if (code != 10) { std::cerr << "UNKNOWN\n"; return 2; }

        // Read model and block it (only main variables 1..m):
        vector<int> model(m+1, 0); // 1-based for convenience
        vector<int> block; block.reserve(m);
        for (int v = 1; v <= m; ++v) {
            int val = S.val(v);              // +v / -v / 0
            model[v] = val;
            block.push_back(val > 0 ? -v : +v);
        }
        ++solutions;

        // Add blocking clause
        for (int lit : block) S.add(lit);
        S.add(0);
    }

    std::cout << solutions << " solutions\n";
    if ((size_t)n < sizeof(expected_count)/sizeof(expected_count[0])) {
        assert(solutions == expected_count[n] && "unexpected count vs OEIS A000088");
    }
    std::cout << "propagate_calls: " << P.propagate_calls << "\n";
    return 0;
}