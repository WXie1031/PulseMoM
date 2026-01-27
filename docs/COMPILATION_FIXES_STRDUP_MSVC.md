# strdup MSVC 编译错误修复

## 问题
- **错误**: `error C4996: 'strdup': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _strdup.`
- **位置**: `src/io/cli/cli_main.c` 第55、61、67行
- **原因**: MSVC 将 POSIX 函数 `strdup` 标记为不安全，建议使用 `_strdup`

## 修复

### src/io/cli/cli_main.c

在文件开头添加条件编译宏，在 MSVC 中将 `strdup` 映射到 `_strdup`:

```c
#include "cli_main.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../api/c_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MSVC compatibility: strdup is deprecated, use _strdup
#if defined(_MSC_VER) && !defined(strdup)
#define strdup _strdup
#endif
```

## 说明

### MSVC 的 strdup 问题

MSVC 将 POSIX 函数（如 `strdup`）标记为不安全，并建议使用带下划线前缀的版本（如 `_strdup`）。这是 MSVC 特有的行为，其他编译器（GCC、Clang）通常支持标准的 `strdup`。

### 跨平台兼容性

使用条件编译确保代码在不同平台上都能编译：

- **MSVC**: 使用 `_strdup`
- **GCC/Clang**: 使用标准的 `strdup`

### 替代方案

项目中 `src/utils/memory_manager.h` 已经定义了 `memory_strdup` 宏：

```c
#ifdef _MSC_VER
#define memory_strdup(s)       _strdup((s))
#else
#define memory_strdup(s)       strdup((s))
#endif
```

但为了保持代码简洁，直接在 `cli_main.c` 中添加条件编译更合适。

## 其他使用 strdup 的文件

项目中还有其他文件使用了 `strdup`，如果它们也遇到类似的编译错误，可以使用相同的方法修复：

- `src/pfd_parser.c`
- `src/core/enhanced_sparameter_extraction.c`
- `src/materials/cst_materials_parser.c`
- `src/core/core_optimization.c`
- `src/solvers/peec/peec_statistical.c`
- `src/solvers/peec/peec_nonlinear.c`
- `src/io/format_validation.c`
- `src/io/advanced_file_formats.c`
- `src/core/circuit_coupling_simulation.c`

## 验证

修复后应该能够编译通过，不再出现 `strdup` 弃用警告。

## 相关文件

- `src/io/cli/cli_main.c` - CLI 主程序
- `src/utils/memory_manager.h` - 内存管理工具（包含 `memory_strdup` 宏）
