#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "logger.h"

#define EH_SUCCESS 0
#define EH_FAILURE -1

#define EH_ASSERT(cond) do { if (!(cond)) { LOG_ERROR("Assertion failed: %s", #cond); abort(); } } while (0)
#define EH_CHECK(cond, msg) do { if (!(cond)) { LOG_ERROR("%s", (msg)); goto EH_ERROR; } } while (0)
#define EH_RETURN_IF_FAIL(cond, msg) do { if (!(cond)) { LOG_ERROR("%s", (msg)); return EH_FAILURE; } } while (0)

#endif /* ERROR_HANDLER_H */
