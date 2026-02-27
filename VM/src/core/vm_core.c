#include "F:\PY\VM\headers\vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void vm_init(VM *vm) {
    for (int i = 0; i < REG_COUNT; i++) { vm->registers[i] = 0; }
    for (int i = 0; i < MEMORY_SIZE; i++) { vm->memory[i] = 0; }
    for (int i = 0; i < STACK_SIZE; i++) { vm->stack[i] = 0; }
    
    vm->sp = -1;
    vm->pc = 0;
    vm->running = 1;
    vm->flags.zero_flag = 0;
    vm->flags.carry_flag = 0;
    vm->flags.sign_flag = 0;
    vm->flags.overflow_flag = 0;
}

void vm_load_prog_input(VM *vm, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return;
    }
    
    memset(vm->memory, 0, MEMORY_SIZE);
    
    size_t bytes_read = fread(vm->memory, 1, MEMORY_SIZE, file);
    fclose(file);
    
    printf("Loaded %zu bytes from %s\n", bytes_read, filename);
    vm->pc = 0;
    vm->running = 1;
}

// void vm_load_prog(VM *vm, uint8_t *prog, size_t prog_size) {
//     for (size_t i = 0; i < MEMORY_SIZE; i++) {
//         vm->memory[i] = 0;
//     }
//     if (prog_size < MEMORY_SIZE) {
//         for (size_t i = 0; i < prog_size; i++) {
//             vm->memory[i] = prog[i];
//         }
//     }
// }

// void vm_run_with_limit(VM *vm, int step_limit) {
//     int steps = 0;
//     while (vm->running && steps < step_limit) {
//         vm_step(vm);
//         steps++;
//     }
//     if (steps >= step_limit) {
//         printf("Step limit of %d reached. Halting VM.\n", step_limit);
//         vm->running = 0;
//     }
// }