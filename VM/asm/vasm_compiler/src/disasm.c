#include <stdio.h>
#include <stdint.h>

#include "../include/common.h"
#include "../include/types.h"
#include "../include/opcodes.h"
#include "../include/disasm.h"

COLD_REGION void disass_vasm(Assembler *asm_ctx) {
    const uint8_t *code = asm_ctx->bytecode;
    int size = asm_ctx->bytecode_pos;
    int pc = 0;

    typedef struct {
        uint8_t opcode;
        const char *mnemonic;
        int fmt;
    } InstrDesc;

    /* fmt legend:
     *  0 - no operands
     *  1 - Rn
     *  2 - Rn, Rm
     *  3 - Rn, Rm, Rk
     *  4 - Rn, imm8
     *  5 - Rn, Rm, imm8
     *  6 - addr16  (jump/call)
     *  7 - Rn, addr16  (LOAD)
     *  8 - addr8, imm8  (READS)
     */
    static const InstrDesc table[] = {
        { OP_HALT,   "HALT",   0 }, { OP_RET,    "RET",    0 },
        { OP_NOP,    "NOP",    0 }, { OP_DBG,    "DBG",    0 },
        { OP_PUSH,   "PUSH",   1 }, { OP_POP,    "POP",    1 },
        { OP_PRINT,  "PRINT",  1 }, { OP_PRINTC, "PRINTC", 1 },
        { OP_PRINTS, "PRINTS", 1 }, { OP_READ,   "READ",   1 },
        { OP_READC,  "READC",  1 },
        { OP_MOV,    "MOV",    2 }, { OP_CMP,    "CMP",    2 },
        { OP_LDB,    "LDB",    2 },
        { OP_ADD,    "ADD",    3 }, { OP_SUB,    "SUB",    3 },
        { OP_MUL,    "MUL",    3 }, { OP_DIV,    "DIV",    3 },
        { OP_XOR,    "XOR",    3 }, { OP_OR,     "OR",     3 },
        { OP_AND,    "AND",    3 }, { OP_SHL,    "SHL",    3 },
        { OP_SHR,    "SHR",    3 }, { OP_STORE,  "STORE",  3 },
        { OP_CMPI,   "CMPI",   4 }, { OP_STOREI, "STOREI", 4 },
        { OP_ADDI,   "ADDI",   5 }, { OP_XORI,   "XORI",   5 },
        { OP_ORI,    "ORI",    5 }, { OP_SHLI,   "SHLI",   5 },
        { OP_SHRI,   "SHRI",   5 },
        { OP_JMP,    "JMP",    6 }, { OP_JE,     "JE",     6 },
        { OP_JNE,    "JNE",    6 }, { OP_JG,     "JG",     6 },
        { OP_JGE,    "JGE",    6 }, { OP_JL,     "JL",     6 },
        { OP_JLE,    "JLE",    6 }, { OP_JNZ,    "JNZ",    6 },
        { OP_CALL,   "CALL",   6 },
        { OP_LOAD,   "LOAD",   7 },
        { OP_READS,  "READS",  8 },
    };
    static const int table_size = sizeof(table) / sizeof(table[0]);

    /* find label name by address (code labels only) */
    #define label_at(addr) ({                                       \
        const char *_n = NULL;                                      \
        for (int _i = 0; _i < asm_ctx->label_count; _i++)           \
            if (!asm_ctx->labels[_i].is_data &&                     \
                asm_ctx->labels[_i].address == (int)(addr))         \
                { _n = asm_ctx->labels[_i].name; break; }           \
        _n; })

    #define MAX_LINES 512
    #define MAX_LINE  128
    char out[MAX_LINES][MAX_LINE];
    int  line_count = 0;

    #define PUSH_LINE(...) do {                                     \
        if (line_count < MAX_LINES)                                 \
            snprintf(out[line_count++], MAX_LINE, __VA_ARGS__);     \
    } while (0)

    snprintf(out[line_count++], MAX_LINE, COLOR_CYAN "Disassembly (%d bytes):" COLOR_RESET, size);
    snprintf(out[line_count++], MAX_LINE, "%-45s", "----------------------------------------------");

    while (pc < size) {
        const char *lbl = label_at(pc);
        if (lbl) PUSH_LINE(COLOR_YELLOW "%s:" COLOR_RESET, lbl);

        int base = pc;
        uint8_t op = code[pc++];

        const InstrDesc *d = NULL;
        for (int i = 0; i < table_size; i++)
            if (table[i].opcode == op) { d = &table[i]; break; }

        if (!d) {
            PUSH_LINE("  %04X | %02X          | " COLOR_RED "??? (0x%02X)" COLOR_RESET, base, op, op);
            continue;
        }

        uint8_t a = (pc < size) ? code[pc] : 0;
        uint8_t b = (pc + 1 < size) ? code[pc + 1] : 0;
        uint8_t c = (pc + 2 < size) ? code[pc + 2] : 0;

        char operands[64] = "";
        switch (d->fmt) {
            case 0: break;
            case 1: snprintf(operands, sizeof(operands), "R%d", a); pc += 1; break;
            case 2: snprintf(operands, sizeof(operands), "R%d, R%d", a, b); pc += 2; break;
            case 3: snprintf(operands, sizeof(operands), "R%d, R%d, R%d", a, b, c); pc += 3; break;
            case 4: snprintf(operands, sizeof(operands), "R%d, %d", a, (int8_t)b); pc += 2; break;
            case 5: snprintf(operands, sizeof(operands), "R%d, R%d, %d", a, b, (int8_t)c); pc += 3; break;
            case 6: {
                uint16_t target = ((uint16_t)a << 8) | b;
                const char *t = label_at(target);
                if (t) snprintf(operands, sizeof(operands), "%s", t);
                else snprintf(operands, sizeof(operands), "0x%04X", target);
                pc += 2;
                break;
            }
            case 7: {
                uint16_t addr = ((uint16_t)b << 8) | c;
                const char *t = label_at(addr);
                if (t) snprintf(operands, sizeof(operands), "R%d, %s", a, t);
                else snprintf(operands, sizeof(operands), "R%d, 0x%02X, 0x%02X", a, b, c);
                pc += 3;
                break;
            }
            case 8: snprintf(operands, sizeof(operands), "0x%02X, %d", a, b); pc += 2; break;
        }

        /* raw bytes column */
        char raw[32] = "";
        int  rp = 0;
        for (int i = base; i < pc; i++)
            rp += snprintf(raw + rp, sizeof(raw) - rp, "%02X ", code[i]);
        for (int i = pc - base; i < 4; i++)
            rp += snprintf(raw + rp, sizeof(raw) - rp, "   ");

        PUSH_LINE("  %04X | %s| %-8s %s", base, raw, d->mnemonic, operands);
    }

    PUSH_LINE("%-45s", "----------------------------------------------");

    /* paged output */
    #define PAGE_SIZE 20
    for (int i = 0; i < line_count; i++) {
        printf("%s\n", out[i]);

        if ((i + 1) % PAGE_SIZE == 0 && (i + 1) < line_count) {
            printf(COLOR_YELLOW "-- [%d/%d] press Enter for next page, q to quit --" COLOR_RESET,
                   i + 1, line_count);
            fflush(stdout);

            int ch = getchar();
            if (ch == 'q' || ch == 'Q') { printf("\n"); break; }
            while (ch != '\n' && ch != EOF) ch = getchar();

            printf("\r\033[K");
        }
    }

    #undef PUSH_LINE
    #undef PAGE_SIZE
    #undef MAX_LINE
    #undef MAX_LINES
    #undef label_at
}