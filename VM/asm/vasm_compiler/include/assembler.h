#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include "types.h"
#include "opcodes.h"

/* utility */
void trim_line(char *line);
int get_register(const char *str);
int parse_number(const char *str);
int check_immediate(Assembler *asm_ctx, ErrorContext *err_ctx, int val, const char *operand);

/* label operations */
int find_label(Assembler *asm_ctx, const char *name);
void add_label(Assembler *asm_ctx, ErrorContext *err_ctx, const char *name, uint16_t address, int is_data);

/* emit */
void emit_byte(Assembler *asm_ctx, ErrorContext *err_ctx, uint8_t byte);
void emit_or_skip(Assembler *a, ErrorContext *e, int pass, uint8_t byte);
void emit_data_byte(Assembler *asm_ctx, ErrorContext *err_ctx, uint8_t byte);

/* parse */
int parse_data_directive(Assembler *asm_ctx, ErrorContext *err_ctx, char *line);
Opcode get_opcode(const char *mnemonic);
int parse_instruction(Assembler *asm_ctx, ErrorContext *err_ctx, char *line, int pass);

#endif /* ASSEMBLER_H */