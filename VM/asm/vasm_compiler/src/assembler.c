#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "../include/common.h"
#include "../include/types.h"
#include "../include/opcodes.h"
#include "../include/error.h"
#include "../include/assembler.h"

/* -------- UTILITY -------- */

COLD_REGION void trim_line(char *line) {
    char *comment = strchr(line, ';');
    if (UNLIKELY(comment)) *comment = '\0';

    int len = strlen(line);
    while (UNLIKELY(len > 0 && isspace((unsigned char)line[len - 1])))
        line[--len] = '\0';
}

FORCE_INLINE HOT_REGION int get_register(const char *str) {
    if (LIKELY(str[0] == 'R' || str[0] == 'r')) {
        int reg = atoi(str + 1);
        if (LIKELY(reg >= 0 && reg <= 7))
            return reg;
    }
    return -1;
}

PURE COLD_REGION int parse_number(const char *str) {
    if (str == NULL || str[0] == '\0') return 0;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        return (int)strtol(str, NULL, 16);
    if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B'))
        return (int)strtol(str + 2, NULL, 2);
    return atoi(str);
}

COLD_REGION int check_immediate(Assembler *asm_ctx, ErrorContext *err_ctx, int val, const char *operand) {
    if (UNLIKELY(val < -128 || val > 255)) {
        error_push(err_ctx, ERR_IMMEDIATE_OVERFLOW, SEVERITY_ERROR,
                   asm_ctx->current_line, 0, asm_ctx->current_source,
                   "immediate value %d out of 8-bit range [-128..255] (operand: '%s')", val, operand);
        return -1;
    }
    return 0;
}

/* -------- LABELS -------- */

COLD_REGION int find_label(Assembler *asm_ctx, const char *name) {
    for (int i = 0; i < asm_ctx->label_count; i++) {
        if (UNLIKELY(strcmp(asm_ctx->labels[i].name, name) == 0))
            return asm_ctx->labels[i].address;
    }
    return -1;
}

COLD_REGION void add_label(Assembler *asm_ctx, ErrorContext *err_ctx,
                            const char *name, uint16_t address, int is_data) {
    if (UNLIKELY(asm_ctx->label_count >= MAX_LABELS)) {
        error_push(err_ctx, ERR_LABEL_TOO_MANY, SEVERITY_FATAL,
                   asm_ctx->current_line, 0, asm_ctx->current_source,
                   "too many labels (max %d)", MAX_LABELS);
        return;
    }

    for (int i = 0; i < asm_ctx->label_count; i++) {
        if (UNLIKELY(strcmp(asm_ctx->labels[i].name, name) == 0)) {
            error_push(err_ctx, WARN_LABEL_DUPLICATE, SEVERITY_WARNING,
                       asm_ctx->current_line, 0, asm_ctx->current_source,
                       "duplicate label '%s'", name);
            return;
        }
    }

    Label *lbl = &asm_ctx->labels[asm_ctx->label_count];
    strncpy(lbl->name, name, sizeof(lbl->name) - 1);
    lbl->name[sizeof(lbl->name) - 1] = '\0';
    lbl->address = address;
    lbl->is_data = is_data;
    asm_ctx->label_count++;
}

/* -------- EMIT -------- */

FORCE_INLINE HOT_REGION void emit_byte(Assembler *asm_ctx, ErrorContext *err_ctx, uint8_t byte) {
    if (UNLIKELY(asm_ctx->bytecode_pos >= MAX_BYTECODE)) {
        error_push(err_ctx, ERR_BYTECODE_OVERFLOW, SEVERITY_FATAL,
                   asm_ctx->current_line, 0, asm_ctx->current_source,
                   "bytecode buffer overflow");
        return;
    }
    asm_ctx->bytecode[asm_ctx->bytecode_pos++] = byte;
}

HOT_REGION void emit_or_skip(Assembler *a, ErrorContext *e, int pass, uint8_t byte) {
    if (pass == 2) emit_byte(a, e, byte);
    else a->bytecode_pos++;
}

COLD_REGION void emit_data_byte(Assembler *asm_ctx, ErrorContext *err_ctx, uint8_t byte) {
    if (UNLIKELY(asm_ctx->data_pos >= MAX_DATA_SECTION)) {
        error_push(err_ctx, ERR_DATA_OVERFLOW, SEVERITY_FATAL,
                   asm_ctx->current_line, 0, asm_ctx->current_source,
                   "data section overflow");
        return;
    }
    asm_ctx->data_section[asm_ctx->data_pos++] = byte;
}

/* -------- DATA DIRECTIVE -------- */

COLD_REGION int parse_data_directive(Assembler *asm_ctx, ErrorContext *err_ctx, char *line) {
    if (UNLIKELY(strncmp(line, ".data", 5) == 0)) { asm_ctx->in_data_section = 1; return 0; }
    if (UNLIKELY(strncmp(line, ".text", 5) == 0)) { asm_ctx->in_data_section = 0; return 0; }
    if (LIKELY(!asm_ctx->in_data_section)) return 0;

    char *colon = strchr(line, ':');
    if (UNLIKELY(!colon)) return 0;
    *colon = '\0';

    char *data_label = line;
    char *data_content = colon + 1;

    while (isspace((unsigned char)*data_label)) data_label++;
    char *end = data_label + strlen(data_label) - 1;
    while (end > data_label && isspace((unsigned char)*end)) *end-- = '\0';
    while (isspace((unsigned char)*data_content)) data_content++;

    if (UNLIKELY(strlen(data_label) == 0)) {
        error_push(err_ctx, ERR_LABEL_EMPTY, SEVERITY_ERROR,
                   asm_ctx->current_line, 0, asm_ctx->current_source,
                   "data label cannot be empty");
        return -1;
    }

    add_label(asm_ctx, err_ctx, data_label, asm_ctx->data_start_addr + asm_ctx->data_pos, 1);

    if (LIKELY(data_content[0] == '"')) {
        data_content++;
        int i = 0;
        while (LIKELY(data_content[i] != '\0') && LIKELY(data_content[i] != '"')) {
            if (UNLIKELY(data_content[i] == '\\')) {
                if (data_content[i+1] == 'n')  { emit_data_byte(asm_ctx, err_ctx, '\n'); i += 2; }
                else if (data_content[i+1] == 't')  { emit_data_byte(asm_ctx, err_ctx, '\t'); i += 2; }
                else if (data_content[i+1] == 'r')  { emit_data_byte(asm_ctx, err_ctx, '\r'); i += 2; }
                else if (data_content[i+1] == '0')  { emit_data_byte(asm_ctx, err_ctx, '\0'); i += 2; }
                else if (data_content[i+1] == '\\') { emit_data_byte(asm_ctx, err_ctx, '\\'); i += 2; }
                else if (data_content[i+1] == '"')  { emit_data_byte(asm_ctx, err_ctx, '"');  i += 2; }
                else {
                    error_push(err_ctx, ERR_ESCAPE_UNKNOWN, SEVERITY_WARNING,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "unknown escape sequence '\\%c' - treating as literal", data_content[i+1]);
                    emit_data_byte(asm_ctx, err_ctx, data_content[i]); i++;
                }
            } else {
                emit_data_byte(asm_ctx, err_ctx, data_content[i]);
                i++;
            }
        }
        emit_data_byte(asm_ctx, err_ctx, '\0');
    } else {
        char *token = strtok(data_content, ",");
        while (LIKELY(token != NULL)) {
            while (isspace((unsigned char)*token)) token++;
            if (LIKELY(strlen(token) > 0))
                emit_data_byte(asm_ctx, err_ctx, parse_number(token) & 0xFF);
            token = strtok(NULL, ",");
        }
    }

    return 0;
}

/* -------- OPCODE LOOKUP -------- */

HOT_REGION Opcode get_opcode(const char *mnemonic) {
    if (LIKELY(strcmp(mnemonic,  "ADD")    == 0)) return OP_ADD;
    if (LIKELY(strcmp(mnemonic,  "ADDI")   == 0)) return OP_ADDI;
    if (LIKELY(strcmp(mnemonic,  "SUB")    == 0)) return OP_SUB;
    if (LIKELY(strcmp(mnemonic,  "MUL")    == 0)) return OP_MUL;
    if (LIKELY(strcmp(mnemonic,  "DIV")    == 0)) return OP_DIV;
    if (LIKELY(strcmp(mnemonic,  "MOV")    == 0)) return OP_MOV;
    if (LIKELY(strcmp(mnemonic,  "CMP")    == 0)) return OP_CMP;
    if (LIKELY(strcmp(mnemonic,  "CMPI")   == 0)) return OP_CMPI;
    if (LIKELY(strcmp(mnemonic,  "LOAD")   == 0)) return OP_LOAD;
    if (UNLIKELY(strcmp(mnemonic, "JMP")   == 0)) return OP_JMP;
    if (UNLIKELY(strcmp(mnemonic, "JE")    == 0)) return OP_JE;
    if (UNLIKELY(strcmp(mnemonic, "JNE")   == 0)) return OP_JNE;
    if (UNLIKELY(strcmp(mnemonic, "JG")    == 0)) return OP_JG;
    if (UNLIKELY(strcmp(mnemonic, "JGE")   == 0)) return OP_JGE;
    if (UNLIKELY(strcmp(mnemonic, "JL")    == 0)) return OP_JL;
    if (UNLIKELY(strcmp(mnemonic, "JLE")   == 0)) return OP_JLE;
    if (UNLIKELY(strcmp(mnemonic, "JNZ")   == 0)) return OP_JNZ;
    if (UNLIKELY(strcmp(mnemonic, "PUSH")  == 0)) return OP_PUSH;
    if (UNLIKELY(strcmp(mnemonic, "POP")   == 0)) return OP_POP;
    if (UNLIKELY(strcmp(mnemonic, "LDB")   == 0)) return OP_LDB;
    if (UNLIKELY(strcmp(mnemonic, "STORE") == 0)) return OP_STORE;
    if (UNLIKELY(strcmp(mnemonic, "STOREI")== 0)) return OP_STOREI;
    if (UNLIKELY(strcmp(mnemonic, "XOR")   == 0)) return OP_XOR;
    if (UNLIKELY(strcmp(mnemonic, "AND")   == 0)) return OP_AND;
    if (UNLIKELY(strcmp(mnemonic, "XORI")  == 0)) return OP_XORI;
    if (UNLIKELY(strcmp(mnemonic, "OR")    == 0)) return OP_OR;
    if (UNLIKELY(strcmp(mnemonic, "ORI")   == 0)) return OP_ORI;
    if (UNLIKELY(strcmp(mnemonic, "SHL")   == 0)) return OP_SHL;
    if (UNLIKELY(strcmp(mnemonic, "SHLI")  == 0)) return OP_SHLI;
    if (UNLIKELY(strcmp(mnemonic, "SHR")   == 0)) return OP_SHR;
    if (UNLIKELY(strcmp(mnemonic, "SHRI")  == 0)) return OP_SHRI;
    if (UNLIKELY(strcmp(mnemonic, "PRINT") == 0)) return OP_PRINT;
    if (UNLIKELY(strcmp(mnemonic, "PRINTC")== 0)) return OP_PRINTC;
    if (UNLIKELY(strcmp(mnemonic, "PRINTS")== 0)) return OP_PRINTS;
    if (UNLIKELY(strcmp(mnemonic, "READ")  == 0)) return OP_READ;
    if (UNLIKELY(strcmp(mnemonic, "READC") == 0)) return OP_READC;
    if (UNLIKELY(strcmp(mnemonic, "READS") == 0)) return OP_READS;
    if (UNLIKELY(strcmp(mnemonic, "RET")   == 0)) return OP_RET;
    if (UNLIKELY(strcmp(mnemonic, "HALT")  == 0)) return OP_HALT;
    if (UNLIKELY(strcmp(mnemonic, "CALL")  == 0)) return OP_CALL;
    if (UNLIKELY(strcmp(mnemonic, "NOP")   == 0)) return OP_NOP;
    if (UNLIKELY(strcmp(mnemonic, "DBG")   == 0)) return OP_DBG;
    return OP_INVALID;
}

/* -------- INSTRUCTION PARSER -------- */

FORCE_INLINE HOT_REGION int parse_instruction(Assembler *asm_ctx, ErrorContext *err_ctx, char *line, int pass) {
    char mnemonic[32];
    char arg1[32], arg2[32], arg3[32];

    if (UNLIKELY(strlen(line) == 0)) return 0;

    /* label on this line? */
    char *colon = strchr(line, ':');
    if (UNLIKELY(colon != NULL)) {
        *colon = '\0';
        if (pass == 1)
            add_label(asm_ctx, err_ctx, line, asm_ctx->bytecode_pos, 0);
        line = colon + 1;
        while (isspace((unsigned char)*line)) line++;
        if (UNLIKELY(strlen(line) == 0)) return 0;
    }

    arg1[0] = '\0';
    arg2[0] = '\0';
    arg3[0] = '\0';

    sscanf(line, "%31s %31[^,],%31[^,],%31[^,]", mnemonic, arg1, arg2, arg3);

    /* strip internal whitespace from operands */
    for (char *p = arg1; *p; p++) if (UNLIKELY(isspace((unsigned char)*p))) memmove(p, p+1, strlen(p)+1);
    for (char *p = arg2; *p; p++) if (UNLIKELY(isspace((unsigned char)*p))) memmove(p, p+1, strlen(p)+1);
    for (char *p = arg3; *p; p++) if (UNLIKELY(isspace((unsigned char)*p))) memmove(p, p+1, strlen(p)+1);

    for (char *p = mnemonic; *p; p++) *p = toupper((unsigned char)*p);

    Opcode opcode = get_opcode(mnemonic);
    if (UNLIKELY(opcode == OP_INVALID && pass == 2)) {
        error_push(err_ctx, ERR_UNKNOWN_INSTRUCTION, SEVERITY_ERROR,
                   asm_ctx->current_line, 0, asm_ctx->current_source,
                   "unknown instruction '%s'", mnemonic);
        return -1;
    }

    /* consecutive NOP warning */
    if (UNLIKELY(opcode == OP_NOP)) {
        if (asm_ctx->last_nop_line > 0 && asm_ctx->last_nop_line == asm_ctx->current_line - 1) {
            error_push(err_ctx, WARN_NOP_SEQUENCE, SEVERITY_WARNING,
                       asm_ctx->current_line, 0, asm_ctx->current_source,
                       "consecutive NOP instructions (lines %d-%d) - intentional?",
                       asm_ctx->last_nop_line, asm_ctx->current_line);
        }
        asm_ctx->last_nop_line = asm_ctx->current_line;
    }

    if (LIKELY(opcode != OP_INVALID)) {
        emit_or_skip(asm_ctx, err_ctx, pass, opcode);

        switch (opcode) {
            /* group 1 - no operands */
            case OP_HALT:
            case OP_RET:
            case OP_NOP:
            case OP_DBG:
                break;

            /* group 2 - single register */
            case OP_PUSH:
            case OP_POP:
            case OP_PRINT:
            case OP_PRINTC:
            case OP_PRINTS:
            case OP_READ:
            case OP_READC: {
                int reg = get_register(arg1);
                if (LIKELY(reg >= 0)) {
                    if (pass == 2) emit_byte(asm_ctx, err_ctx, reg);
                    else asm_ctx->bytecode_pos++;
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "invalid register '%s'", arg1);
                    return -1;
                }
                break;
            }

            /* group 3 - jump/call: 2-byte address */
            case OP_JMP:
            case OP_JE:
            case OP_JNE:
            case OP_JG:
            case OP_JGE:
            case OP_JL:
            case OP_JLE:
            case OP_JNZ:
            case OP_CALL: {
                int addr = find_label(asm_ctx, arg1);
                if (UNLIKELY(addr < 0))
                    addr = parse_number(arg1);

                if (UNLIKELY(pass == 2 && arg1[0] == '\0')) {
                    error_push(err_ctx, ERR_OPERAND_MISSING, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "'%s' requires a label or address operand", mnemonic);
                    return -1;
                }
                if (UNLIKELY(pass == 2 && (addr < 0 || addr >= MAX_BYTECODE))) {
                    error_push(err_ctx, ERR_JUMP_OUT_OF_RANGE, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "jump target 0x%04X is out of valid range [0x0000..0x%04X]",
                               addr, MAX_BYTECODE - 1);
                    return -1;
                }
                if (UNLIKELY(pass == 2 && addr == asm_ctx->bytecode_pos + 2)) {
                    error_push(err_ctx, WARN_JUMP_NEXT, SEVERITY_WARNING,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "jump target is the next instruction (0x%04X) — has no effect", addr);
                }
                if (pass == 2) {
                    emit_byte(asm_ctx, err_ctx, (addr >> 8) & 0xFF);
                    emit_byte(asm_ctx, err_ctx, addr & 0xFF);
                } else {
                    asm_ctx->bytecode_pos += 2;
                }
                break;
            }

            /* group 4 - two registers */
            case OP_MOV:
            case OP_CMP: {
                int reg1 = get_register(arg1);
                int reg2 = get_register(arg2);
                if (LIKELY(reg1 >= 0 && reg2 >= 0)) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, err_ctx, reg1);
                        emit_byte(asm_ctx, err_ctx, reg2);
                    } else {
                        asm_ctx->bytecode_pos += 2;
                    }
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "invalid registers '%s', '%s'", arg1, arg2);
                    return -1;
                }
                break;
            }

            /* group 5 - register + immediate/label */
            case OP_CMPI:
            case OP_STOREI: {
                int reg = get_register(arg1);
                int label_addr = find_label(asm_ctx, arg2);
                int val = (UNLIKELY(label_addr >= 0)) ? label_addr : parse_number(arg2);

                if (UNLIKELY(pass == 2 && arg2[0] == '\0')) {
                    error_push(err_ctx, ERR_OPERAND_MISSING, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "'%s' requires two operands: register and immediate/label", mnemonic);
                    return -1;
                }
                if (UNLIKELY(pass == 2 && label_addr < 0))
                    check_immediate(asm_ctx, err_ctx, val, arg2);

                if (LIKELY(reg >= 0)) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, err_ctx, reg);
                        emit_byte(asm_ctx, err_ctx, val & 0xFF);
                    } else {
                        asm_ctx->bytecode_pos += 2;
                    }
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "invalid register '%s'", arg1);
                    return -1;
                }
                break;
            }

            /* group 5b - LDB Rdest, Raddr */
            case OP_LDB: {
                int reg_dest = get_register(arg1);
                int reg_addr = get_register(arg2);
                if (LIKELY(reg_dest >= 0 && reg_addr >= 0)) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, err_ctx, reg_dest);
                        emit_byte(asm_ctx, err_ctx, reg_addr);
                    } else {
                        asm_ctx->bytecode_pos += 2;
                    }
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "LDB requires two registers: LDB Rdest, Raddr");
                    return -1;
                }
                break;
            }

            /* group 6 - READS addr, maxlen */
            case OP_READS: {
                int addr = parse_number(arg1);
                int maxlen = parse_number(arg2);
                if (pass == 2) {
                    emit_byte(asm_ctx, err_ctx, addr & 0xFF);
                    emit_byte(asm_ctx, err_ctx, maxlen & 0xFF);
                } else {
                    asm_ctx->bytecode_pos += 2;
                }
                break;
            }

            /* group 7 - three registers */
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_XOR:
            case OP_OR:
            case OP_AND:
            case OP_SHL:
            case OP_SHR:
            case OP_STORE: {
                int reg1 = get_register(arg1);
                int reg2 = get_register(arg2);
                int reg3 = get_register(arg3);
                if (LIKELY(reg1 >= 0 && reg2 >= 0 && reg3 >= 0)) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, err_ctx, reg1);
                        emit_byte(asm_ctx, err_ctx, reg2);
                        emit_byte(asm_ctx, err_ctx, reg3);
                    } else {
                        asm_ctx->bytecode_pos += 3;
                    }
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "invalid registers '%s', '%s', '%s'", arg1, arg2, arg3);
                    return -1;
                }
                break;
            }

            /* group 8 - two registers + immediate */
            case OP_ADDI:
            case OP_XORI:
            case OP_ORI:
            case OP_SHLI:
            case OP_SHRI: {
                int reg1 = get_register(arg1);
                int reg2 = get_register(arg2);
                int val  = parse_number(arg3);

                if (UNLIKELY(pass == 2 && arg3[0] == '\0')) {
                    error_push(err_ctx, ERR_OPERAND_MISSING, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "'%s' requires three operands: Rdest, Rsrc, immediate", mnemonic);
                    return -1;
                }
                if (UNLIKELY(pass == 2))
                    check_immediate(asm_ctx, err_ctx, val, arg3);

                if (LIKELY(reg1 >= 0 && reg2 >= 0)) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, err_ctx, reg1);
                        emit_byte(asm_ctx, err_ctx, reg2);
                        emit_byte(asm_ctx, err_ctx, val & 0xFF);
                    } else {
                        asm_ctx->bytecode_pos += 3;
                    }
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "invalid registers '%s', '%s'", arg1, arg2);
                    return -1;
                }
                break;
            }

            /* group 9 - LOAD reg, label | reg, high, low */
            case OP_LOAD: {
                int reg = get_register(arg1);
                int val_high, val_low;

                int label_addr = find_label(asm_ctx, arg2);
                if (UNLIKELY(label_addr >= 0)) {
                    val_high = (label_addr >> 8) & 0xFF;
                    val_low = label_addr & 0xFF;
                } else {
                    val_high = parse_number(arg2);
                    val_low = parse_number(arg3);
                }

                if (LIKELY(reg >= 0)) {
                    if (pass == 2) {
                        emit_byte(asm_ctx, err_ctx, reg);
                        emit_byte(asm_ctx, err_ctx, val_high & 0xFF);
                        emit_byte(asm_ctx, err_ctx, val_low & 0xFF);
                    } else {
                        asm_ctx->bytecode_pos += 3;
                    }
                } else if (UNLIKELY(pass == 2)) {
                    error_push(err_ctx, ERR_INVALID_REGISTER, SEVERITY_ERROR,
                               asm_ctx->current_line, 0, asm_ctx->current_source,
                               "invalid register '%s'", arg1);
                    return -1;
                }
                break;
            }
        }
    }

    return 0;
}