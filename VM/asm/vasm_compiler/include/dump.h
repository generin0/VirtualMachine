#ifndef DUMP_H
#define DUMP_H

#include "types.h"

COLD_REGION void help_print(char *argv[]);
COLD_REGION void debug_hex(Assembler *asm_ctx);
COLD_REGION void dump_labels(Assembler *asm_ctx);
COLD_REGION void dump_data_section(Assembler *asm_ctx);

#endif /* DUMP_H */