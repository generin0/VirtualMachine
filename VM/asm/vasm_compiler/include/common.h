#ifndef COMMON_H
#define COMMON_H

/* -------- LIMITS -------- */
#define MAX_LABELS        256
#define MAX_LINE_LENGTH   256
#define MAX_BYTECODE      1024
#define MAX_DATA_SECTION  256
#define MAX_ERRORS        64

/* -------- COLORS -------- */
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"

/* -------- OPTIMIZATIONS -------- */
#define LIKELY(x)    __builtin_expect(!!(x), 1)
#define UNLIKELY(x)  __builtin_expect(!!(x), 0)

#define HOT_REGION   __attribute__((hot))
#define COLD_REGION  __attribute__((cold))
#define FORCE_INLINE inline __attribute__((always_inline))
#define PURE         __attribute__((pure))

#endif /* COMMON_H */