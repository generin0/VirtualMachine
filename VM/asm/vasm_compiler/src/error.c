#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../include/common.h"
#include "../include/types.h"
#include "../include/error.h"

COLD_REGION void error_dump(ErrorContext *ctx) {
    if (ctx->count == 0) return;

    fprintf(stderr, COLOR_RED "\nCompilation result: %d error(s), %d warning(s)\n" COLOR_RESET,
            ctx->error_count, ctx->warning_count);

    for (int i = 0; i < ctx->count; i++) {
        Error *err = &ctx->errors[i];

        const char *tag;
        const char *color;
        switch (err->severity) {
            case SEVERITY_WARNING: tag = "WARN "; color = COLOR_YELLOW; break;
            case SEVERITY_ERROR: tag = "ERROR"; color = COLOR_RED; break;
            case SEVERITY_FATAL: tag = "FATAL"; color = COLOR_RED; break;
            default: tag = "?????"; color = COLOR_YELLOW; break;
        }

        if (err->line > 0)
            fprintf(stderr, "%s[%s]" COLOR_RESET " line %-4d : %s\n", color, tag, err->line, err->message);
        else
            fprintf(stderr, "%s[%s]" COLOR_RESET " : %s\n", color, tag, err->message);

        if (err->source_line[0] != '\0') {
            fprintf(stderr, COLOR_CYAN "  %4d | " COLOR_RESET "%s\n", err->line, err->source_line);
            if (err->col > 0) {
                fprintf(stderr, "       | ");
                for (int c = 0; c < err->col - 1; c++) fprintf(stderr, " ");
                fprintf(stderr, "%s^" COLOR_RESET "\n", color);
            }
        }
    }
}

COLD_REGION void error_push(ErrorContext *ctx, ErrorCode code, Severity severity,
                             int line, int col, const char *source_line,
                             const char *fmt, ...) {
    if (UNLIKELY(ctx->count >= MAX_ERRORS)) {
        fprintf(stderr, "Error: too many errors (max %d), cannot log more\n", MAX_ERRORS);
        return;
    }

    Error *err = &ctx->errors[ctx->count++];
    err->code = code;
    err->line = line;
    err->col  = col;
    err->severity = severity;

    if (source_line)
        strncpy(err->source_line, source_line, sizeof(err->source_line) - 1);
    else
        err->source_line[0] = '\0';

    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);

    if (severity == SEVERITY_WARNING) ctx->warning_count++;
    else if (severity == SEVERITY_ERROR)  ctx->error_count++;

    if (UNLIKELY(severity == SEVERITY_FATAL)) {
        ctx->has_fatal = 1;
        fprintf(stderr, COLOR_RED "[FATAL]" COLOR_RESET " line %d: %s\n", line, err->message);
        if (source_line && source_line[0]) {
            fprintf(stderr, "  | %s\n", source_line);
            if (col > 0) {
                fprintf(stderr, "  | ");
                for (int i = 0; i < col - 1; i++) fprintf(stderr, " ");
                fprintf(stderr, COLOR_RED "^" COLOR_RESET "\n");
            }
        }
        error_dump(ctx);
        exit(1);
    }
}

HOT_REGION int error_has_errors(ErrorContext *ctx) {
    return ctx->error_count > 0;
}