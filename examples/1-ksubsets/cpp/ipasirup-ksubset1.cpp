// exactly_k_extprop_api21.cpp
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cadical.hpp"   // -I<cadical>/src

using std::vector;
using std::unordered_map;
using std::unordered_set;

static unsigned long long binom(unsigned n, unsigned k){
    if (k>n) return 0ULL; if (k>n-k) k=n-k;
    unsigned long long r=1; for (unsigned i=1;i<=k;++i) r = (r*(n-k+i))/i; return r;
}

struct ChooseKProp : public CaDiCaL::ExternalPropagator {
    CaDiCaL::Solver* S = nullptr;
    const int n, k;

    // Level-Stacks und Zustände
    size_t level = 0;
    vector<unordered_set<int>> assigned_at_level{1}; // signed lits je Level
    vector<unordered_set<int>> reason_lits_at_level{1};
    unordered_set<int> pos, neg; // belegte Variablen (Indices) positiv/negativ

    // Propagation: FIFO von implizierten Literalen
    std::queue<int> prop_queue;
    // Reason-Klauseln, on-demand
    unordered_map<int, vector<int>> reason_of; // implied_lit -> clause

    // Externe (zusätzliche) Klauseln (z.B. Blocking-Clauses); 0 terminiert
    std::queue<vector<int>> ext_clauses;

    // Statistik
    unsigned long long solutions = 0;

    ChooseKProp(int n_, int k_) : n(n_), k(k_) {}

    // ---------- Hilfsfunktionen ----------
    bool is_unassigned_var(int v) const { return !pos.count(v) && !neg.count(v); }

    void enqueue_implication(int lit, const vector<int>& reason) {
        if (!reason_of.count(lit)) {
            prop_queue.push(lit);
            reason_of[lit] = reason;
            reason_lits_at_level[level].insert(lit);
        }
    }

    // AtMost(k): wenn k TRUE, Rest muss FALSE werden
    void at_most_k_propagate() {
        if ((int)pos.size() != k) return;
        vector<int> prefix; prefix.reserve(k+1);
        vector<int> tmp(pos.begin(), pos.end());
        std::sort(tmp.begin(), tmp.end());
        for (int t : tmp) prefix.push_back(-t);
        for (int v = 1; v <= n; ++v)
            if (is_unassigned_var(v)) {
                auto clause = prefix; clause.push_back(-v);
                enqueue_implication(-v, clause);
            }
    }

    // AtLeast(k): wenn n-k FALSE, Rest muss TRUE werden
    void at_least_k_propagate() {
        if ((int)neg.size() != (n - k)) return;
        vector<int> prefix; prefix.reserve((n-k)+1);
        vector<int> tmp(neg.begin(), neg.end());
        std::sort(tmp.begin(), tmp.end());
        for (int t : tmp) prefix.push_back(+t);
        for (int v = 1; v <= n; ++v)
            if (is_unassigned_var(v)) {
                auto clause = prefix; clause.push_back(+v);
                enqueue_implication(+v, clause);
            }
    }

    // ---------- ExternalPropagator-Schnittstelle ----------

    // Solver verbindet den Propagator
    void init(CaDiCaL::Solver* solver) { S = solver; } // kein override; nur Helfer

    void notify_new_decision_level() override {
        ++level;
        assigned_at_level.emplace_back();
        reason_lits_at_level.emplace_back();
    }

    void notify_backtrack(size_t new_level) override {
        while (level > new_level) {
            for (int lit : assigned_at_level[level]) {
                if (lit > 0) pos.erase(lit); else neg.erase(-lit);
            }
            assigned_at_level.pop_back();
            for (int lit : reason_lits_at_level[level]) reason_of.erase(lit);
            reason_lits_at_level.pop_back();
            --level;
        }
        // ausstehende propagationen verwerfen (CaDiCaL fragt cb_propagate neu)
        while (!prop_queue.empty()) prop_queue.pop();
    }

    // Batch neuer Trail-Zuweisungen
    void notify_assignment(const vector<int>& lits) override {
        for (int lit : lits) {
            assigned_at_level[level].insert(lit);
            if (lit > 0) pos.insert(lit); else neg.insert(-lit);
        }
        at_most_k_propagate();
        at_least_k_propagate();
    }

    // Ein Literal zur Propagation zurückgeben (0 = nichts)
    int cb_propagate() override {
        if (prop_queue.empty()) return 0;
        int lit = prop_queue.front(); prop_queue.pop();
        return lit;
    }

    // Reason-Klausel für ein von uns propagiertes Literal – Lits nacheinander liefern; 0 = Ende
    int cb_add_reason_clause_lit(int propagated_lit) override {
        auto it = reason_of.find(propagated_lit);
        if (it == reason_of.end()) return 0;
        auto& cls = it->second;
        if (cls.empty()) { reason_of.erase(it); return 0; }
        int lit = cls.back(); cls.pop_back();
        return lit;
    }

    // (optional) eigene Entscheidungen – hier nicht genutzt
    int cb_decide() override { return 0; }

    // Volles Modell prüfen (wie dein check_model)
    bool cb_check_found_model(const vector<int>& model) override {
        int t = 0;
        vector<int> block; block.reserve(n);
        for (int v = 1; v <= n; ++v) {
            int val = model[v-1]; // ±v
            if (val > 0) ++t;
            block.push_back(val > 0 ? -v : +v);
        }
        if (t != k) {
            // Falsches Modell: blockieren und ablehnen
            ext_clauses.push(block);
            return false;
        }
        // Gültige Lösung: zählen & blockieren (Enumerierung)
        ++solutions;
        ext_clauses.push(block);
        return true;
    }

    // Externe Klauseln (Blocking, evtl. weitere)
    bool cb_has_external_clause(bool &is_forgettable) override {
        is_forgettable = true; // darf vom Solver später vergessen werden
        return !ext_clauses.empty();
    }

    int cb_add_external_clause_lit() override {
        static vector<int> cur;
        if (cur.empty()) {
            if (ext_clauses.empty()) return 0;
            cur = std::move(ext_clauses.front());
            ext_clauses.pop();
        }
        if (cur.empty()) return 0;
        int lit = cur.back(); cur.pop_back();
        if (cur.empty()) return 0; // 0 signalisiert Klausel-Ende
        return lit;
    }
};

int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: " << argv[0] << " n k\n"; return 1; }
    int n = std::atoi(argv[1]), k = std::atoi(argv[2]);
    assert(0 <= k && k <= n);

    CaDiCaL::Solver solver;
    ChooseKProp prop(n, k);
    solver.connect_external_propagator(&prop);
    prop.init(&solver);

    // beobachtete Variablen registrieren (sonst kommen keine Callbacks)
    for (int v = 1; v <= n; ++v) solver.add_observed_var(v);

    // leere CNF – Propagator macht die Musik
    while (true) {
        int res = solver.solve(); // 10 SAT, 20 UNSAT
        if (res == 20) break;
        if (res != 10) { std::cerr << "UNKNOWN\n"; return 2; }
        // Bei SAT triggert CaDiCaL cb_check_found_model(...) → dort blocken wir
    }

    std::cout << prop.solutions << " solutions, expected: C(" << n << "," << k
              << ") = " << binom(n,k) << "\n";
    return 0;
}