#include "F:\PY\VM\headers\vm.h"

/*
 *   OP_FLAG_ADD  = 0
 *   OP_FLAG_SUB  = 1
 *   OP_FLAG_MUL  = 2
 *   OP_FLAG_DIV  = 3
 *   OP_FLAG_AND  = 4
 *   OP_FLAG_OR   = 5
 *   OP_FLAG_XOR  = 6
 *   OP_FLAG_SHL  = 7   (shift_amount passed via b)
 *   OP_FLAG_SHR  = 8   (shift_amount passed via b, original value via a)
 *   OP_FLAG_MOV  = 9
 *   OP_FLAG_LOAD = 10
 *   OP_FLAG_LDB  = 11
 *   OP_FLAG_POP  = 12
 */

void set_flags_after_operation(VM *vm, int32_t result, uint32_t a, uint32_t b, uint8_t operation) {
    /* ZF */
    vm->flags.zero_flag = (result == 0);

    /* SF */
    vm->flags.sign_flag = (result < 0);

    switch (operation) {

        /*ADD*/
        case 0: {
            // CF
            vm->flags.carry_flag = ((uint64_t)a + (uint64_t)b) > 0xFFFFFFFF;
            // OF
            int32_t sa = (int32_t)a, sb = (int32_t)b;
            vm->flags.overflow_flag = ((sa > 0 && sb > 0 && result <= 0) ||
                                       (sa < 0 && sb < 0 && result >= 0));
            break;
        }

        /* SUB / CMP */
        case 1: {
            // CF
            vm->flags.carry_flag = (a < b);
            // OF
            int32_t sa = (int32_t)a, sb = (int32_t)b;
            vm->flags.overflow_flag = ((sa >= 0 && sb < 0 && result < 0) ||
                                       (sa < 0  && sb >= 0 && result > 0));
            break;
        }

        /* MUL */
        case 2: {
            int64_t full = (int64_t)(int32_t)a * (int64_t)(int32_t)b;
            uint8_t overflow = (full != (int64_t)result);
            vm->flags.carry_flag    = overflow;
            vm->flags.overflow_flag = overflow;
            break;
        }

        /* DIV */
        case 3: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /* AND */
        case 4: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /* OR */
        case 5: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /* XOR */
        case 6: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /* SHL */
        case 7: {
            uint32_t shift = b & 0x1F;
            // CF
            vm->flags.carry_flag = (shift > 0) ? ((a >> (32 - shift)) & 1) : 0;
            // OF
            vm->flags.overflow_flag = (shift == 1) ? (vm->flags.carry_flag ^ (uint8_t)vm->flags.sign_flag) : 0;
            break;
        }

        /* SHR */
        case 8: {
            uint32_t shift = b & 0x1F;
            // CF
            vm->flags.carry_flag = (shift > 0) ? ((a >> (shift - 1)) & 1) : 0;
            // OF
            vm->flags.overflow_flag = (shift == 1) ? ((a >> 31) & 1) : 0;
            break;
        }

        /* MOV */
        case 9: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /* LOAD */
        case 10: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /* LDB */
        case 11: {
            // SF
            vm->flags.sign_flag = ((int8_t)(result & 0xFF) < 0);
            vm->flags.carry_flag = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        /*  POP  */
        case 12: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }

        default: {
            vm->flags.carry_flag    = 0;
            vm->flags.overflow_flag = 0;
            break;
        }
    }
}