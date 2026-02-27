#ifndef VM_H
#define VM_H

#include <stdint.h>

#define MEMORY_SIZE 1024 // 1024 bytes of memory from 0x00 to 0x3FF
#define REG_COUNT 8 // 8 register
#define STACK_SIZE 64 // stack size of 64 integers

enum Opcodes {
    OP_HALT = 0x00,
    OP_ADD = 0x01,
    OP_ADDI = 0x02,
    OP_SUB = 0x03,
    OP_MUL = 0x04,
    OP_DIV = 0x05,
    OP_MOV = 0x06,
    OP_CMP = 0x07,
    OP_JMP = 0x08,
    OP_JE = 0x09,
    OP_JG = 0x0A,
    OP_JNZ = 0x0B,
    OP_PUSH = 0x0C,
    OP_POP = 0x0D,
    OP_LOAD = 0x0E,
    OP_XOR = 0x0F,
    OP_XORI = 0x10,
    OP_SHL = 0x11,
    OP_SHLI = 0x12,
    OP_SHR = 0x13,
    OP_SHRI = 0x14,
    OP_STORE = 0x15,
    OP_CALL = 0x16,
    OP_RET = 0x17,
    OP_STOREI = 0x18,
    OP_PRINT = 0x19,
    OP_PRINTC = 0x1A,
    OP_READ = 0x1B,
    OP_READC = 0x1C,
    OP_READS = 0x1D,
    OP_JL = 0x1E,
    OP_JLE = 0x1F,
    OP_JGE = 0x20,
    OP_JNE = 0x21,
    OP_LDB = 0x22, 
    OP_PRINTS = 0x23,
    OP_CMPI = 0x24,
    OP_AND = 0x25,
    OP_OR = 0x26,
    OP_ORI = 0x27,

    OP_NOP  = 0x60, /*Special*/
    OP_DBG  = 0xFF  /*opcodes*/
};

typedef struct {
    uint8_t zero_flag;
    uint8_t carry_flag;
    uint8_t sign_flag;
    uint8_t overflow_flag;
} flags_t;

typedef struct { // main vm struct
    flags_t flags;
    uint8_t memory[MEMORY_SIZE];
    int8_t sp; // stack pointer
    uint8_t running;
    uint16_t pc; // program count, current opcode
    uint32_t registers[REG_COUNT];
    int32_t stack[STACK_SIZE];
} VM;

void vm_init(VM *vm);
//void vm_load_prog(VM *vm, uint8_t *prog, size_t prog_size);
void vm_load_prog_input(VM *vm, const char *filename);
void vm_step(VM *vm);
void vm_run_with_limit(VM *vm, int step_limit);
void set_flags_after_operation(VM *vm, int32_t result, uint32_t a, uint32_t b, uint8_t operation);
void vm_dbg(VM *vm);

#endif