# Architecture Layers

## Layer Mapping

- Physics: solvers/mom, solvers/peec, solvers/mtl
- Discretization: mesh, geometry, basis_functions
- Operators: kernels, integration, assembler, matvec
- Backend: solver, gpu, fast algorithms
- Orchestration: hybrid_solver, workflows
- IO/API: io, api, gui, python_interface

## Rules
- Lower layers must not depend on higher layers
- Backend must not embed physics semantics
- Orchestration owns execution order and data flow


# ARCHITECTURE_DECISION_TREE.md
## Where Should This Code Live?

---

## 0. Purpose

This document defines a **mandatory decision tree** for placing
new code, refactoring existing code, and reviewing contributions.

Its goal is to ensure:
- architectural consistency
- long-term maintainability
- solver independence
- prevention of layer pollution

**All new code MUST be placed by following this decision tree.**

---

## 1. First Question: What Is the Nature of This Code?

### Q1. Does this code define a physical law, equation, or constitutive relation?

Examples:
- Maxwell equations (integral / differential form)
- PEEC partial inductance definitions
- Material constitutive relations
- Boundary condition formulations

✅ YES → `physics/` (L1)  
❌ NO → go to Q2

---

## 2. Is This Code a Numerical Discretization of Physics?

### Q2. Does this code convert physical equations into algebraic form?

Examples:
- Basis function definitions (RWG, pulse, rooftop)
- Weak-form assembly
- Matrix stamping rules
- Operator definition (not execution)

✅ YES → `operators/` (L2 / L3)  
❌ NO → go to Q3

---

## 3. Does This Code Change the Mathematical Meaning of the Operator?

### Q3. Does this code approximate, compress, or reformulate an operator?

Examples:
- ACA / H-matrix approximation models
- Low-rank representations
- Block structure definitions
- Preconditioner mathematics

✅ YES → `operators/operator_approximation/` (L3)  
❌ NO → go to Q4

---

## 4. Does This Code Only Change *How* an Operator Is Executed?

### Q4. Is the mathematical operator unchanged, but execution differs?

Examples:
- GPU kernels
- Parallel matvec
- Memory layout optimization
- Task scheduling
- SIMD / CUDA / OpenMP logic

✅ YES → `backend/` (L4)  
❌ NO → go to Q5

---

## 5. Is This Code Making Decisions About *When* or *Which* Solver to Use?

### Q5. Does this code decide:
- which solver to invoke
- frequency vs time-domain
- wideband vs single-frequency
- hybrid solver coupling
- iteration order or workflow

✅ YES → `orchestration/` (L5)  
❌ NO → go to Q6

---

## 6. Is This Code Specific to a Particular Solver?

### Q6. Is this logic only meaningful for a specific solver (MoM, PEEC, MTL, Ray)?

Examples:
- MoM-specific assembly pipelines
- PEEC time stepping
- MTL propagation routines
- Ray tracing engines

✅ YES → `solvers/<solver_name>/`  
❌ NO → go to Q7

---

## 7. Is This Code About Interoperability Between Solvers?

### Q7. Does this code exchange physical quantities across solvers?

Examples:
- Field handoff (Ray → FDTD)
- Current/voltage coupling
- Domain decomposition interfaces
- Multi-physics adapters

✅ YES → `interop/` (Adapters only)  
❌ NO → go to Q8

---

## 8. Is This Code Purely Infrastructure or Utility?

### Q8. Does this code have *no physical or numerical meaning*?

Examples:
- Logging
- Configuration parsing
- File I/O
- Memory pools
- Timing utilities

✅ YES → `utils/` or `common/`  
❌ NO → go to Q9

---

## 9. Is This Code Defining Rules, Constraints, or Guarantees?

### Q9. Is this code or document enforcing consistency?

Examples:
- Unit normalization
- Result validation
- Regression tests
- Numerical sanity checks
- Version compatibility

✅ YES → `governance/`  
❌ NO → STOP and re-evaluate design

---

## 10. Forbidden Shortcuts (Hard Rules)

The following are **architectural violations**:

- Placing solver-selection logic inside `solvers/`
- Calling backend kernels from `physics/`
- Embedding physical assumptions inside `utils/`
- Letting one solver include headers from another solver
- Bypassing adapters for solver coupling

Violations MUST be refactored.

---

## 11. Tie-Breaking Rule

If code seems to belong to multiple layers:

> **Place it in the HIGHEST layer that depends on the others,  
> but is not depended upon by them.**

Never push code downward to “simplify dependencies”.

---

## 12. Refactoring Guidance

When refactoring existing code:

1. Identify its *highest-level responsibility*
2. Split mixed-responsibility files
3. Move orchestration logic upward
4. Preserve solver cores as pure executors

---

## 13. Why This Matters

Following this decision tree ensures:

- New solvers can be added without refactoring old ones
- Numerical optimizations do not change physics
- Physical correctness is not compromised by performance work
- Long-term evolution remains controlled

Ignoring this decision tree will inevitably lead to:
- tangled dependencies
- untestable solvers
- silent physical errors

---

## 14. Final Rule

> **If you cannot clearly answer these questions,  
> the code is not ready to be written.**

This decision tree is mandatory for all future development.
