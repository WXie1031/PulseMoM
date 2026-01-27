# Memory Rules

## Scope
Memory pools and ownership management. These belong to **L4 Numerical Backend Layer**.

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ Manages memory pools for efficient allocation
- ✅ Provides explicit ownership semantics
- ✅ Tracks memory usage
- ❌ Does NOT leak memory
- ❌ Does NOT use hidden allocations

## Core Constraints
- **Explicit ownership**: All memory ownership is explicit
- **No hidden allocation**: All allocations are visible
- **Memory safety**: No leaks, no double-free
- **Efficient**: Uses memory pools for performance

## Memory Management Features

### Memory Pools
- **Status**: ✅ Implemented
- **Location**: `src/backend/memory/memory_pool.c`
- **Features**:
  - Pool-based allocation
  - Efficient reuse
  - Memory tracking

### Ownership Semantics
- **Creator owns**: Function that allocates owns the memory
- **Explicit transfer**: Ownership transfer is explicit
- **Clear lifecycle**: Creation and destruction are clear

## Implementation Status

| Feature | Status | Location | Notes |
|---------|--------|----------|-------|
| Memory pools | ✅ | `memory_pool.c` | Fully implemented |
| Ownership tracking | ✅ | Throughout codebase | Explicit patterns |
| Memory validation | ⚠️ | Basic | Could be enhanced |

## File Locations

- **Memory Pool**: `src/backend/memory/memory_pool.c`
- **Memory Interface**: `src/backend/memory/memory_pool.h`

## Usage

### Memory Pool
```c
memory_pool_t* pool = memory_pool_create(initial_size);
void* ptr = memory_pool_allocate(pool, size);
// ... use memory ...
memory_pool_free(pool, ptr);
memory_pool_destroy(pool);
```

### Explicit Ownership
```c
// Creator owns
operator_matrix_t* matrix = matrix_assembler_create_matrix(type, m, n);

// Explicit transfer (if needed)
// Caller takes ownership
void* geometry = geometry_engine_create();

// Explicit destruction
matrix_assembler_destroy_matrix(matrix);
geometry_engine_destroy(geometry);
```

## Core Constraints

- **Explicit ownership**: All ownership is clear
- **No hidden allocation**: All allocations visible
- **Memory safety**: No leaks, proper cleanup
- **Efficient**: Use pools for performance-critical paths
