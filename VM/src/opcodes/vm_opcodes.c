#include "F:\PY\VM\headers\vm.h"
#include <stdio.h>
#include <string.h>

void vm_step(VM *vm) {
    if (!vm || vm->pc >= MEMORY_SIZE) {
        printf("PC out of bounds or VM is NULL.\n");
        vm->running = 0;
        return;
    }

    uint8_t opcode = vm->memory[vm->pc++];
    uint8_t pc_before = vm->pc - 1;
    //printf("DEBUG: PC=%02X opcode=%02X\n", vm->pc-1, opcode);

    switch (opcode) {
        case OP_DBG: {
            vm_dbg(vm);
            break;
        }

        case OP_NOP: {
            //printf("[%02X] NOP\n", pc_before);
            break;
        }

        case OP_HALT: {
            vm->running = 0;
            //printf("[%02X] HALT\n", pc_before);
            break;
        }

        case OP_PRINT: {
            uint8_t reg = vm->memory[vm->pc++];
            if (reg < REG_COUNT) {
                printf("%d", vm->registers[reg]);
            }
            break;
        }

        case OP_PRINTC: {
            uint8_t reg = vm->memory[vm->pc++];
            if (reg < REG_COUNT) {
                putchar((char)vm->registers[reg]);
            }
            break;
        }

        case OP_PRINTS: {
            uint8_t reg_addr = vm->memory[vm->pc++];
            if (reg_addr < REG_COUNT) {
                uint16_t addr = vm->registers[reg_addr];
                if (addr < MEMORY_SIZE) {
                    for (uint16_t i = 0; addr + i < MEMORY_SIZE; i++) {
                        char ch = vm->memory[addr + i];
                        if (ch == 0) break;
                        putchar(ch);
                    }
                }
            }
            break;
        }

        case OP_AND: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t result = vm->registers[reg_src1] & vm->registers[reg_src2];
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, vm->registers[reg_src1], vm->registers[reg_src2], 4);
                //printf("[%02X] AND R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_XOR: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t result = vm->registers[reg_src1] ^ vm->registers[reg_src2];
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, vm->registers[reg_src1], vm->registers[reg_src2], 6);
                //printf("[%02X] XOR R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_XORI: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT) {
                uint32_t result = vm->registers[reg_src1] ^ imm;
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, vm->registers[reg_src1], (uint32_t)imm, 6);
                //printf("[%02X] XORI R%d,R%d,#%d\n", pc_before, reg_dest, reg_src1, imm);
            }
            break;
        }

        case OP_OR: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t result = vm->registers[reg_src1] | vm->registers[reg_src2];
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, vm->registers[reg_src1], vm->registers[reg_src2], 5);
                //printf("[%02X] OR R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_ORI: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT) {
                uint32_t result = vm->registers[reg_src1] | imm;
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, vm->registers[reg_src1], (uint32_t)imm, 5);
                //printf("[%02X] OR R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_SHL: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t shift_amount = vm->registers[reg_src2] & 0x1F;
                uint32_t value = vm->registers[reg_src1];
                uint32_t result = value << shift_amount;
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, value, shift_amount, 7);
                //printf("[%02X] SHL R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_SHLI: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT) {
                uint8_t shift_amount = imm & 0x1F;
                uint32_t value = vm->registers[reg_src1];
                uint32_t result = value << shift_amount;
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, value, (uint32_t)shift_amount, 7);
                //printf("[%02X] SHLI R%d,R%d,#%d\n", pc_before, reg_dest, reg_src1, shift_amount);
            }
            break;
        }

        case OP_SHR: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t shift_amount = vm->registers[reg_src2] & 0x1F;
                uint32_t value = vm->registers[reg_src1];
                uint32_t result = value >> shift_amount;
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, value, shift_amount, 8);
                //printf("[%02X] SHR R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_SHRI: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT) {
                uint8_t shift_amount = imm & 0x1F;
                uint32_t value = vm->registers[reg_src1];
                uint32_t result = value >> shift_amount;
                vm->registers[reg_dest] = result;
                set_flags_after_operation(vm, (int32_t)result, value, (uint32_t)shift_amount, 8);
                //printf("[%02X] SHRI R%d,R%d,#%d\n", pc_before, reg_dest, reg_src1, shift_amount);
            }
            break;
        }

        case OP_ADD: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t a = vm->registers[reg_src1];
                uint32_t b = vm->registers[reg_src2];
                int32_t result = (int32_t)a + (int32_t)b;
                vm->registers[reg_dest] = (uint32_t)result;
                set_flags_after_operation(vm, result, a, b, 0);
                //printf("[%02X] ADD R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_ADDI: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT) {
                uint32_t a = vm->registers[reg_src1];
                uint32_t b = imm;
                int32_t result = (int32_t)a + (int32_t)b;
                vm->registers[reg_dest] = (uint32_t)result;
                set_flags_after_operation(vm, result, a, b, 0);
                //printf("[%02X] ADDI R%d,R%d,#%d\n", pc_before, reg_dest, reg_src1, imm);
            }
            break;
        }

        case OP_SUB: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                uint32_t a = vm->registers[reg_src1];
                uint32_t b = vm->registers[reg_src2];
                int32_t result = (int32_t)a - (int32_t)b;
                vm->registers[reg_dest] = (uint32_t)result;
                set_flags_after_operation(vm, result, a, b, 1);
                //printf("[%02X] SUB R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_MUL: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                int64_t result64 = (int64_t)(int32_t)vm->registers[reg_src1] * 
                                   (int64_t)(int32_t)vm->registers[reg_src2];
                int32_t result32 = (int32_t)result64;
                vm->registers[reg_dest] = (uint32_t)result32;
                set_flags_after_operation(vm, result32, vm->registers[reg_src1], vm->registers[reg_src2], 2);
                //printf("[%02X] MUL R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_DIV: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src1 = vm->memory[vm->pc++];
            uint8_t reg_src2 = vm->memory[vm->pc++];
            if (vm->registers[reg_src2] == 0) {
                //printf("[%02X] DIV ERR\n", pc_before);
                vm->running = 0;
                return;
            }
            if (reg_dest < REG_COUNT && reg_src1 < REG_COUNT && reg_src2 < REG_COUNT) {
                int32_t result = (int32_t)vm->registers[reg_src1] / (int32_t)vm->registers[reg_src2];
                vm->registers[reg_dest] = (uint32_t)result;
                set_flags_after_operation(vm, result, vm->registers[reg_src1], vm->registers[reg_src2], 3);
                //printf("[%02X] DIV R%d,R%d,R%d\n", pc_before, reg_dest, reg_src1, reg_src2);
            }
            break;
        }

        case OP_MOV: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_src = vm->memory[vm->pc++];
            if (reg_dest < REG_COUNT && reg_src < REG_COUNT) {
                vm->registers[reg_dest] = vm->registers[reg_src];
                set_flags_after_operation(vm, (int32_t)vm->registers[reg_dest], vm->registers[reg_dest], 0, 9);
                //printf("[%02X] MOV R%d,R%d\n", pc_before, reg_dest, reg_src);
            }
            break;
        }

        case OP_CMP: {
            uint8_t reg1 = vm->memory[vm->pc++];
            uint8_t reg2 = vm->memory[vm->pc++];
            if (reg1 < REG_COUNT && reg2 < REG_COUNT) {
                uint32_t a = vm->registers[reg1];
                uint32_t b = vm->registers[reg2];
                int32_t result = (int32_t)a - (int32_t)b;
                set_flags_after_operation(vm, result, a, b, 1);
                //printf("[%02X] CMP R%d,R%d\n", pc_before, reg1, reg2);
            }
            break;
        }

        case OP_CMPI: {
            uint8_t reg1 = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg1 < REG_COUNT) {
                uint32_t a = vm->registers[reg1];
                uint32_t b = imm;
                int32_t result = (int32_t)a - (int32_t)b;
                set_flags_after_operation(vm, result, a, b, 1);
                //printf("[%02X] CMPI R%d,#%d\n", pc_before, reg1, imm);
            }
            break;
        }

        case OP_LOAD: {
            uint8_t reg = vm->memory[vm->pc++];
            if (vm->pc + 1 >= MEMORY_SIZE) {
                //printf("[%02X] LOAD ERR\n", pc_before);
                vm->running = 0;
                return;
            }
            int32_t value = (vm->memory[vm->pc] << 8) | vm->memory[vm->pc + 1];
            vm->pc += 2;
            if (reg < REG_COUNT) {
                vm->registers[reg] = value;
                set_flags_after_operation(vm, value, (uint32_t)value, 0, 10);
                //printf("[%02X] LOAD R%d\n", pc_before, reg);
            }
            break;
        }

        case OP_LDB: {
            uint8_t reg_dest = vm->memory[vm->pc++];
            uint8_t reg_addr = vm->memory[vm->pc++];
            
            if (reg_dest < REG_COUNT && reg_addr < REG_COUNT) {
                uint16_t addr = vm->registers[reg_addr];
                if (addr < MEMORY_SIZE) {
                    vm->registers[reg_dest] = vm->memory[addr];
                    set_flags_after_operation(vm, (int32_t)vm->registers[reg_dest], vm->registers[reg_dest], 0, 11);
                    //printf("[0x%02X] LDB  R%d, [R%d]  ; R%d = memory[0x%04X] = 0x%02X\n", pc_before, reg_dest, reg_addr, reg_dest, addr, vm->registers[reg_dest]);
                }
            }
            break;
        }

        case OP_STORE: {
            uint8_t reg = vm->memory[vm->pc++];
            uint8_t addr = vm->memory[vm->pc++];
            if (reg < REG_COUNT && addr < MEMORY_SIZE) {
                union {
                    uint32_t u32;
                    uint8_t bytes[4];
                } data;
                data.u32 = vm->registers[reg];
                for (int i = 0; i < 4; i++) {
                    vm->memory[addr + i] = data.bytes[3 - i];
                }
                //printf("[%02X] STORE R%d,[%02X]\n", pc_before, reg, addr);
            }
            break;
        }

        case OP_STOREI: {
            uint8_t reg = vm->memory[vm->pc++];
            uint8_t imm = vm->memory[vm->pc++];
            if (reg < REG_COUNT && imm + 3 < MEMORY_SIZE) {
                union {
                    uint32_t u32;
                    uint8_t bytes[4];
                } data;
                data.u32 = vm->registers[reg];
                for (int i = 0; i < 4; i++) {
                    vm->memory[imm + i] = data.bytes[3 - i];
                }
                //printf("[%02X] STOREI R%d,[#%02X]\n", pc_before, reg, imm);
            }
            break;
        }

        case OP_READ: {
            uint8_t reg = vm->memory[vm->pc++];
            if (reg < REG_COUNT) {
                uint32_t value;
                scanf("%d", &value);
                vm->registers[reg] = value;
                printf("\n");
            }
            break;
        }

        case OP_READC: {
            uint8_t reg = vm->memory[vm->pc++];
            if (reg < REG_COUNT) {
                int ch = getchar();
                if (ch == '\n') {
                    ch = getchar();
                }
                vm->registers[reg] = ch;
                printf("\n");
            }
            break;
        }

        case OP_READS: {
            uint8_t addr = vm->memory[vm->pc++];
            uint8_t max_len = vm->memory[vm->pc++];
            if (addr < MEMORY_SIZE && addr + max_len < MEMORY_SIZE) {
                char buffer[256];
                fgets(buffer, max_len, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                for (size_t i = 0; i < strlen(buffer) && i < max_len; i++) {
                    if (addr + i < MEMORY_SIZE) {
                        vm->memory[addr + i] = buffer[i];
                    }
                }
                if (addr + strlen(buffer) < MEMORY_SIZE) {
                    vm->memory[addr + strlen(buffer)] = '\0';
                }
                printf("\n");
            }
            break;
        }

        case OP_CALL: {
            uint8_t addr = vm->memory[vm->pc++];
            if (addr < MEMORY_SIZE && vm->sp < STACK_SIZE - 1) {
                vm->stack[++vm->sp] = vm->pc;
                vm->pc = addr;
                //printf("[%02X] CALL %02X\n", pc_before, addr);
            }
            break;
        }

        case OP_RET: {
            if (vm->sp >= 0) {
                vm->pc = vm->stack[vm->sp--];
                //printf("[%02X] RET\n", pc_before);
            } else {
                //printf("[%02X] RET ERR\n", pc_before);
                vm->running = 0;
            }
            break;
        }

        case OP_PUSH: {
            uint8_t reg = vm->memory[vm->pc++];
            if (reg < REG_COUNT && vm->sp < STACK_SIZE - 1) {
                vm->stack[++vm->sp] = vm->registers[reg];
                //printf("[%02X] PUSH R%d\n", pc_before, reg);
            } else if (vm->sp >= STACK_SIZE - 1) {
                //printf("[%02X] PUSH ERR\n", pc_before);
                vm->running = 0;
            }
            break;
        }

        case OP_POP: {
            uint8_t reg = vm->memory[vm->pc++];
            if (reg < REG_COUNT && vm->sp >= 0) {
                vm->registers[reg] = vm->stack[vm->sp--];
                set_flags_after_operation(vm, (int32_t)vm->registers[reg], vm->registers[reg], 0, 12);
                //printf("[%02X] POP R%d\n", pc_before, reg);
            } else if (vm->sp < 0) {
                //printf("[%02X] POP ERR\n", pc_before);
                vm->running = 0;
            }
            break;
        }

        case OP_JNZ: {
            uint8_t addr = vm->memory[vm->pc++];
            if (!vm->flags.zero_flag && addr < MEMORY_SIZE) {
                //printf("[%02X] JNZ %02X\n", pc_before, addr);
                vm->pc = addr;
            } else {
                //printf("[%02X] JNZ skip\n", pc_before);
            }
            break;
        }

        case OP_JE: {
            uint8_t addr = vm->memory[vm->pc++];
            if (vm->flags.zero_flag && addr < MEMORY_SIZE) {
                //printf("[%02X] JE %02X\n", pc_before, addr);
                vm->pc = addr;
            } else {
                //printf("[%02X] JE skip\n", pc_before);
            }
            break;
        }

        case OP_JNE: {
            uint8_t addr = vm->memory[vm->pc++];
            //printf("[0x%02X] JNE  #0x%02X", pc_before, addr);
            if (!vm->flags.zero_flag && addr < MEMORY_SIZE) {
                //printf("  ; TAKEN -> PC=0x%02X\n", addr);
                vm->pc = addr;
            } else {
                //printf("  ; NOT TAKEN (ZF=%d)\n", vm->flags.zero_flag);
            }
            break;
        }

        case OP_JG: {
            uint8_t addr = vm->memory[vm->pc++];
            if (!vm->flags.zero_flag && (vm->flags.sign_flag == vm->flags.overflow_flag) && addr < MEMORY_SIZE) {
                //printf("[%02X] JG %02X\n", pc_before, addr);
                vm->pc = addr;
            } else {
                //printf("[%02X] JG skip\n", pc_before);
            }
            break;
        }

        case OP_JGE: {  // Jump if Greater or Equal (SF == OF)
            uint8_t addr = vm->memory[vm->pc++];
            //printf("[0x%02X] JGE  #0x%02X", pc_before, addr);
            if (vm->flags.sign_flag == vm->flags.overflow_flag && addr < MEMORY_SIZE) {
                //printf("  ; TAKEN -> PC=0x%02X\n", addr);
                vm->pc = addr;
            } else {
                //printf("  ; NOT TAKEN (SF=%d, OF=%d)\n", vm->flags.sign_flag, vm->flags.overflow_flag);
            }
            break;
        }

        case OP_JL: {
            uint8_t addr = vm->memory[vm->pc++];
            if (vm->flags.sign_flag != vm->flags.overflow_flag && addr < MEMORY_SIZE) {
                //printf("[%02X] JL %02X\n", pc_before, addr);
                vm->pc = addr;
            } else {
                //printf("[%02X] JL skip\n", pc_before);
            }
            break;
        }

        case OP_JLE: {
            uint8_t addr = vm->memory[vm->pc++];
            //printf("[0x%02X] JLE  #0x%02X", pc_before, addr);
            if (vm->flags.zero_flag || (vm->flags.sign_flag != vm->flags.overflow_flag)) {
                //printf("  ; TAKEN -> PC=0x%02X\n", addr); 
                vm->pc = addr;
            } else {
                //printf("  ; NOT TAKEN (ZF=%d, SF=%d, OF=%d)\n", vm->flags.zero_flag, vm->flags.sign_flag, vm->flags.overflow_flag);
            }
            break;
        }

        case OP_JMP: {
            uint8_t addr = vm->memory[vm->pc++]; // Непосредственный адрес (1 байт)
            vm->pc = addr;
            //printf("JMP to address %d\n", addr);
            break;
        }

        default: {
            printf("[%02X] UNKNOWN\n", pc_before);
            vm->running = 0;
            break;
        }
    }
}