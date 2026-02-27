#include "F:\PY\VM\headers\vm.h"
#include <stdio.h>

void vm_dbg(VM *vm) {
    printf("\n");
    printf("PC: %02X  SP: %d\n", vm->pc, vm->sp);
    printf("Flags: Z=%d S=%d C=%d O=%d\n", 
           vm->flags.zero_flag, vm->flags.sign_flag,
           vm->flags.carry_flag, vm->flags.overflow_flag);
    
    printf("\nRegisters:\n");
    for(int i = 0; i < REG_COUNT; i++) {
        printf("R%d: %08X (%d)\n", i, vm->registers[i], (int32_t)vm->registers[i]);
    }
    
    if (vm->sp < 0) {
        printf("\nStack: empty\n");
    } else {
        printf("\nStack:\n");
        for(int i = vm->sp, j = 0; i >= 0 && j < 8; i--, j++) {
            printf("[%d] %d\n", i, vm->stack[i]);
        }
    }

    printf("\nMemory (PC):\n");
    for(int i = vm->pc - 4; i < vm->pc + 8 && i < MEMORY_SIZE; i++) {
        if(i >= 0) {
            printf("%02X ", vm->memory[i]);
        }
    }
    printf("\n");
    printf("\n");
}