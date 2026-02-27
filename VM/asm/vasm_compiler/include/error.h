#ifndef ERROR_H
#define ERROR_H

#include "types.h"

void error_push(ErrorContext *ctx, ErrorCode code, Severity severity,
                int line, int col, const char *source_line,
                const char *fmt, ...);

void error_dump(ErrorContext *ctx);
int error_has_errors(ErrorContext *ctx);

#endif /* ERROR_H */