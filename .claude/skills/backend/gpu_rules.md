# Skill: GPU Backend Rules

## Scope
GPU is a numerical backend, not a solver.

## Core Constraints
- GPU code must not assume MoM/PEEC/MTL semantics
- GPU kernels operate on abstract operators or matrices

## Mandatory Patterns
- Backend selection via capability flags
- CPU/GPU result equivalence checks

## Forbidden Patterns
- Physics-specific branching in GPU kernels
- Solver-specific logic inside GPU code

## Rationale
Physics leakage into GPU kernels prevents backend reuse and breaks hybrid coupling.
