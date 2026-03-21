# Memory Footprint Reduction for Dense and Fast-Multipole MoM Solvers in PulseMoM

## 中文摘要

矩量法（MoM）离散表面积分方程得到的复阻抗矩阵 **Z** 为 *N*×*N*，完整存储需 Θ(*N*²) 内存；工程实现中常因 LU 副本、LAPACK 列主序转置等出现 **约两倍** 峰值占用。本文从 **理论复杂度** 出发，归纳 PulseMoM 中已实现的 **原地 LU**、**LAPACK 成功后尽早释放 Z**、**按问题规模自动选择算法**，以及 MLFMM 路径下用 **CSR 仅存近场非零元** 替代稠密 *N*×*N* 矩阵等措施，将近场块空间由 Θ(*N*²) 降为 Θ(nnz)，并说明与既有“近场稠密、远场简化”数值模型的一致性边界及后续工作方向。

**关键词：** 矩量法；空间复杂度；近场稀疏；CSR；原地分解；算法自动选择

---

## Abstract

The Method of Moments (MoM) discretization of surface integral equations yields a dense complex impedance matrix **Z** of dimension *N*×*N*, where *N* is the number of basis functions. Storing **Z** in full requires Θ(*N*²) memory; practical implementations often exceed a naive “16*N*² bytes” estimate because auxiliary buffers (e.g., a second matrix for LAPACK column-major layout, or a duplicate for LU factorization) temporarily double the working set. This note summarizes **theoretical complexity**, **engineering changes** implemented in PulseMoM, and **remaining asymptotic limits**.

**Keywords:** Method of Moments, memory complexity, sparse near-field, CSR, in-place LU, algorithm auto-selection.

---

## 1. Introduction

For frequency-domain MoM with *N* unknowns, the linear system

\[
\mathbf{Z}\mathbf{I}=\mathbf{V},\qquad \mathbf{Z}\in\mathbb{C}^{N\times N}
\]

is the dominant memory consumer when **Z** is stored densely. Fast algorithms (MLFMM, H-matrix, ACA) aim to reduce both **time** and **space** by approximating far-field interactions. In many production codes, however, a “fast” path still allocates a full dense **Z** for simplicity, which negates memory savings.

---

## 2. Complexity Model

### 2.1 Dense direct solve

- **Storage (complex entries):** Θ(*N*²). If each entry uses 16 bytes (two `double`), RAM ≈ **16*N*²** bytes for **Z** alone.
- **Gaussian elimination / LU:** Classical implementations may hold **Z** and a **copy** for factors during setup, giving a **peak** of ≈ **2×16*N*²** before releasing one buffer.
- **LAPACK `zgesv` (column-major):** A row-major assembled **Z** is often **transposed** into a second buffer **A**, again producing a **2×** spike until **Z** is released on the success path.

### 2.2 Iterative Krylov methods

Matrix–vector products **y = Zx** require either:

- **Dense Z:** Θ(*N*²) memory, Θ(*N*²) time per product; or  
- **Compressed or sparse structure:** memory Θ(*S*) with *S* ≪ *N*² when far interactions are not stored explicitly.

### 2.3 Near-field sparsity (distance screening)

Green’s function interactions decay with distance (and oscillate). A common **practical** approximation is:

\[
Z_{ij} \approx 0 \quad \text{if} \quad \|\mathbf{r}_{c,i}-\mathbf{r}_{c,j}\| > d_{\mathrm{near}}
\]

(except for self-terms and nearby pairs). This yields a **sparse** near-field block; **nnz** (number of nonzeros) scales roughly as **O(*N*·*k*)** where *k* is the average number of neighbors within *d*ₙₑₐᵣ, often **O(1)** to **O(log N)** depending on mesh and threshold.

**CSR (Compressed Sparse Row)** stores **nnz** values with **(n+1) + nnz** integers for row pointers and column indices:

\[
\text{RAM}_{\mathrm{CSR}} = (n+1 + \texttt{nnz})\cdot\texttt{sizeof(int)} + \texttt{nnz}\cdot\texttt{sizeof(complex\_t)}.
\]

For large *N* and moderate *k*, **nnz** ≪ *N*², giving **sub-quadratic** matrix storage.

---

## 3. Implemented Optimizations (PulseMoM)

### 3.1 In-place LU (basic direct path)

**Problem:** A duplicate **LU** buffer of size *N*² was allocated and filled by `memcpy` from **Z**.

**Remedy:** Perform partial-pivoting LU **in place** on the assembled **Z**.  

**Effect:** Peak memory for the basic path drops from **≈2*N*²** complex entries to **≈1*N*²** plus **O(*N*)** workspace (pivots, substitution vector).

**Correctness:** Same numerical sequence of eliminations on the same data layout as before the copy; only the redundant buffer was removed.

### 3.2 LAPACK path

**Remedy:** On successful `zgesv`, release the original row-major **Z** (`impedance_matrix`) immediately after the solve so the process does not carry two full matrices until the outer cleanup.

**Note:** During transpose/factorization, a second *N*×*N* buffer may still be required unless the assembler writes column-major directly (future work).

### 3.3 CLI / unified solver: algorithm auto-selection

**Problem:** With `config == NULL`, `mom_solve_unified` defaulted to `MOM_ALGO_BASIC` and **never** called `select_algorithm`, forcing dense direct solve for all *N*.

**Remedy:** If `config` is NULL, or non-NULL with default BASIC, call `select_algorithm(problem)` after `compute_problem_characteristics`.

**Policy (current):**

| *N* | Selected algorithm |
|-----|---------------------|
| *N* < 600 | `MOM_ALGO_BASIC` (dense direct) |
| 600 ≤ *N* < 200000 | `MOM_ALGO_MLFMM` |
| larger / very large electrical size | `MOM_ALGO_MLFMM` or `MOM_ALGO_HMATRIX` |

This steers medium/large problems away from Θ(*N*²) **dense factorization** toward **iterative** driver + structured matvec.

### 3.4 MLFMM assembly: CSR near-field instead of dense *N*×*N*

**Problem:** The MLFMM branch still did `calloc(N*N)` and filled far entries with zero—**Θ(*N*²)** RAM for mostly zeros.

**Remedy:** Two-pass construction:

1. **Count** near pairs per row *i* (same distance test as before: \(r < d_{\mathrm{near}}\) or *i = j*).  
2. **Prefix-sum** into `row_ptr`, allocate `col_ind` and `values`, **fill** CSR in parallel (row-disjoint writes).

**Matvec:** `mom_csr_complex_matvec` implements **y = Z_CSR x** in Θ(**nnz**) time.

**Effect:** Matrix storage drops from **16*N*²** bytes (dense) to **≈16·nnz** bytes (values) plus index overhead, typically **O(*N*)–O(*N* log *N*)** when *d*ₙₑₐᵣ is fixed in wavelengths.

**Model consistency:** The previous MLFMM matvec used dense **Z** with zeros outside the near block; far-field was not fully applied in that simplified path. CSR omits explicit zeros but **does not by itself add** far-field correction—accuracy profile matches the prior “near-only dense row” model until multipole matvec is wired to the same interface.

---

## 4. Discussion and Limits

1. **Asymptotic lower bound for true dense MoM:** Any **exact** dense factorization requires Ω(*N*²) input storage unless **Z** is never formed (matrix-free) or is compressed with controlled error.  
2. **Iterative convergence:** Replacing direct solve by iterative refinement/GMRES changes **spectral** behavior and may require preconditioning; tolerance and iteration caps affect accuracy.  
3. **Symmetry:** If **Z** is symmetric (or Hermitian), packed storage and specialized factorizations can save ~50%; PulseMoM’s EFIE assembly exploits symmetry in **fill** but still used full storage for LU—further reduction would need packed + appropriate solver.  
4. **True MLFMM:** Optimal implementations avoid storing the full far-field; they add far contributions via multipole/local expansions in **O(*N* log *N*)** time per matvec with **O(*N*)** hierarchical storage.

---

## 5. Conclusion

We reduced **peak RAM** by (i) **eliminating redundant dense copies** in the direct solver, (ii) **releasing Z early** after successful LAPACK solves, (iii) **auto-selecting** non-dense algorithms for default CLI runs, and (iv) storing the MLFMM **near-field block in CSR**, yielding **Θ(nnz)** matrix memory instead of **Θ(*N*²)** for that block. Together, these changes align the implementation’s **space complexity** more closely with standard fast-MoM practice while preserving the previous numerical model for the near-field block.

---

## References (conceptual)

1. R. F. Harrington, *Field Computation by Moment Methods*, Wiley-IEEE Press.  
2. W. C. Chew et al., *Fast and Efficient Algorithms in Computational Electromagnetics*, Artech House.  
3. Y. Saad, *Iterative Methods for Sparse Linear Systems*, SIAM.  
4. S. M. Rao, D. R. Wilton, A. W. Glisson, “Electromagnetic scattering by surfaces of arbitrary shape,” *IEEE Trans. Antennas Propag.*, 1982.

---

*Document version: aligned with PulseMoM repository changes (CSR MLFMM near-field, in-place LU, auto algorithm selection).*

### Implementation note (basis dimension)

The unified triangle–triangle assembler indexes rows/columns by **triangle index** (`0 … num_elements−1`). The linear system dimension **N** must therefore be **`num_elements`**, not `num_edges`. Using **N = num_edges** when `num_edges > num_elements` leaves most rows of **Z** zero and breaks CSR/iterative solves (very small **nnz** relative to **N**). The CLI/minimal solver sets `num_unknowns = num_elements` accordingly; VTK export uses a per-triangle magnitude path when `num_unknowns == num_elements`, and the full RWG edge reconstruction when dimensions match an edge-based coefficient vector.

**Mesh units:** Near-field distance tests must use the same length unit as vertex coordinates. If coordinates are in **millimetres** but λ = *c*/*f* is in **metres**, a naive comparison `r < 0.1λ` makes **nnz ≈ N** (almost diagonal only) and the iterative MLFMM path fails. The implementation infers **metres per coordinate unit** (e.g. mm → 10⁻³) for the MLFMM CSR cutoff and warns when **nnz** is too small; on iterative failure it **falls back to dense BASIC** direct solve for moderate **N**.
