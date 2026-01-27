# Skill: Hybrid Solver Orchestration

## Scope
Hybrid solver acts as execution orchestrator, not numerical implementation.

## Core Constraints
- Hybrid solver must not implement physics kernels
- Hybrid solver must not directly modify mesh or basis
- Hybrid solver owns execution order and coupling graph

## Mandatory Patterns
- Explicit operator registration
- Explicit data flow definition
- Backend-agnostic invocation

## Forbidden Patterns
- Direct calls into mom/peec numerical internals
- Implicit solver state mutation
- Hidden coupling via global variables

## Rationale
Previous implementations caused silent coupling bugs and made GPU/CPU paths diverge.

## Safe Modification Checklist
- Does this change alter execution order?
- Does this introduce hidden coupling?
