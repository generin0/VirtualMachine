#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "common.h"

/* -------- ERROR TYPES -------- */

typedef enum {
    ERR_NONE = 0,
    ERR_UNKNOWN_INSTRUCTION,
    ERR_INVALID_REGISTER,
    ERR_INVALID_OPERAND,
    ERR_WRONG_OPERAND_COUNT,
    ERR_OPERAND_MISSING,
    ERR_IMMEDIATE_OVERFLOW,
    ERR_JUMP_OUT_OF_RANGE,
    ERR_ESCAPE_UNKNOWN,

    ERR_LABEL_TOO_MANY,
    ERR_LABEL_DUPLICATE,
    ERR_LABEL_NOT_FOUND,
    ERR_LABEL_EMPTY,

    ERR_BYTECODE_OVERFLOW,
    ERR_DATA_OVERFLOW,

    ERR_FILE_OPEN,
    ERR_FILE_WRITE,

    WARN_LABEL_DUPLICATE,
    WARN_NOP_SEQUENCE,
    WARN_JUMP_NEXT,
} ErrorCode;

typedef enum {
    SEVERITY_WARNING,
    SEVERITY_ERROR,
    SEVERITY_FATAL,
} Severity;

typedef struct {
    ErrorCode code;
    Severity severity;
    char message[256];
    char source_line[MAX_LINE_LENGTH];
    int line;
    int col;
} Error;

typedef struct {
    Error errors[MAX_ERRORS];
    int count;
    int warning_count;
    int error_count;
    int has_fatal;
} ErrorContext;

/* -------- ASSEMBLER TYPES -------- */

typedef struct {
    char name[64];
    uint16_t address;
    int is_data;
} Label;

typedef struct {
    Label labels[MAX_LABELS];
    uint8_t bytecode[MAX_BYTECODE];
    uint8_t data_section[MAX_DATA_SECTION];
    int label_count;
    int bytecode_pos;
    int data_pos;
    int in_data_section;
    int data_start_addr;
    int current_line;
    char current_source[MAX_LINE_LENGTH];
    int last_nop_line;
} Assembler;

/* -------- CLI TYPES -------- */

typedef struct {
    int debug_mode;
    int dump_labels;
    int silent;
    int dump_data;
    int disass;
} InputArguments;

#endif /* TYPES_H */