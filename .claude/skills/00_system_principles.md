# System Principle: Separation of Concerns

PulseMoM enforces strict separation between:

1. Physics definition (MoM / PEEC / MTL)
2. Discretization (mesh, basis, geometry)
3. Operator assembly (kernel, integration, matrix)
4. Numerical backend (solver, GPU, fast algorithms)
5. Execution orchestration (hybrid solver, workflows)

No module may cross layers implicitly.

Violations are considered architectural bugs.
