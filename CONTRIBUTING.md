# Contributions & Engineering Manifesto (api-haven)

This project is a strictly solo development process conducted in tight pair-programming partnership with an AI coding assistant.

It serves as an architectural manifesto for a **zero-GC, zero-allocation Java systems library**, bridging high-level languages to raw C23 memory layouts and native POSIX/macOS primitives.

---

## 1. The AI-First Architecture Manifesto & Boilerplate Defense

This codebase strictly enforces the verbose, explicit boilerplate required across the `vexgraph` ecosystem:
- Single Class Per File (the Java Law).
- Absolute zero allocation in tick, layout, or event loops (no hidden object instantiations).
- Complete, symmetric getters and setters for all records and off-heap views.
- Off-heap raw memory manipulation via direct addresses rather than wrapper objects.

### Why the Boilerplate Exists
This boilerplate is **not** an accident, nor is it a misunderstanding of idiomatic Java. It is an intentional, machine-verifiable scaffold built specifically for **AI-Human Pair Systems Programming**:
1. **Machine Comprehension**: Explicit off-heap pointers and manual layout offsets allow an AI coding agent to verify alignment and struct field mapping with zero ambiguity.
2. **AI-Maintained Rigor**: The AI agent writes and maintains repetitive off-heap accessor boilerplate, eliminating human typing toil while ensuring zero GC pressure.

---

## 2. Sanity Warning for External Contributors

> [!WARNING]
> **SANITY NOTICE FOR EXTERNAL CONTRIBUTORS**
> This repository is not designed for traditional Java conveniences, casual hacking, or stylistic shortcuts. It is an unapologetic, machine-verifiable manifesto of AI-augmented zero-GC systems architecture.
>
> **If you do not approve of this architecture or cannot find peace with this philosophy, consider leaving this repository for your own sanity.**
>
> We do not accept Pull Requests, issues, or unsolicited stylistic refactors attempting to re-introduce garbage-collected wrappers or bypass zero-allocation invariants. Upstream is maintained exclusively by the author and the AI agent.

---

## 3. Supreme Living Document: `preferences.md`

All architectural rules and style invariants are governed by the central constitution:

- **[preferences.md](https://github.com/vexgraph-dev/vexspoke/blob/main/preferences.md)** (tracked in `vexspoke`, accessible locally at `../../preferences.md`)

Whenever preferences or conventions evolve, `preferences.md` is updated and committed locally in the same cycle (Zero Drift Law).

