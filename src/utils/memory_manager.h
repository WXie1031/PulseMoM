#ifndef PULSEEM_UTILS_MEMORY_MANAGER_H
#define PULSEEM_UTILS_MEMORY_MANAGER_H

#include <stdlib.h>

#define mm_malloc(size)       malloc((size))
#define mm_calloc(n, size)    calloc((n), (size))
#define mm_realloc(ptr, size) realloc((ptr), (size))
#define mm_free(ptr)          free((ptr))

/* Legacy aliases to avoid int-to-pointer casts warnings in old subsystems */
#define memory_allocate(size)  mm_malloc((size))
#define memory_callocate(n,sz) mm_calloc((n),(sz))
#define memory_reallocate(p,s) mm_realloc((p),(s))
#define memory_free(ptr)       mm_free((ptr))
#ifdef _MSC_VER
#define memory_strdup(s)       _strdup((s))
#else
#define memory_strdup(s)       strdup((s))
#endif

#endif /* PULSEEM_UTILS_MEMORY_MANAGER_H */
