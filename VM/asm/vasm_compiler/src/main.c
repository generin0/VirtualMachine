#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/common.h"
#include "../include/types.h"
#include "../include/error.h"
#include "../include/assembler.h"
#include "../include/disasm.h"
#include "../include/dump.h"

HOT_REGION int main(int argc, char *argv[]) {
    InputArguments input_args = {0};

    if (UNLIKELY(argc < 2)) {
        printf("Usage: %s <input_file.vasm> <output_file.bin> <-flag>\n", argv[0]);
        return 1;
    }

    /* parse flags */
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug"))  input_args.debug_mode = 1;
        else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--vasm")) input_args.disass = 1;
        else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--labels")) input_args.dump_labels = 1;
        else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--silent")) input_args.silent = 1;
        else if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--data")) input_args.dump_data = 1;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { help_print(argv); return 0; }
    }

    ErrorContext err_ctx = {0};

    if (UNLIKELY(!input_args.disass && argc < 3)) {
        printf("Usage: %s <input_file.vasm> <output_file.bin> <-flag>\n", argv[0]);
        return 1;
    }

    /* validate file extensions */
    size_t len_vasm = strlen(argv[1]);
    if (UNLIKELY(len_vasm < 5 || strcmp(argv[1] + len_vasm - 5, ".vasm") != 0)) {
        error_push(&err_ctx, ERR_FILE_OPEN, SEVERITY_FATAL, 0, 0, NULL,
                   "input file must have .vasm extension");
        return 1;
    }

    if (!input_args.disass) {
        size_t len_bin = strlen(argv[2]);
        if (UNLIKELY(len_bin < 4 || strcmp(argv[2] + len_bin - 4, ".bin") != 0)) {
            error_push(&err_ctx, ERR_FILE_OPEN, SEVERITY_FATAL, 0, 0, NULL,
                       "output file must have .bin extension");
            return 1;
        }
    }

    Assembler asm_ctx = {0};
    asm_ctx.data_start_addr = 0x0100;

    FILE *input = fopen(argv[1], "r");
    if (UNLIKELY(!input)) {
        error_push(&err_ctx, ERR_FILE_OPEN, SEVERITY_FATAL, 0, 0, NULL,
                   "could not open file '%s'", argv[1]);
        return 1;
    }

    char line[MAX_LINE_LENGTH];

    /* ---- pass 1: collect labels ---- */
    if (LIKELY(!input_args.silent))
        printf("First pass - collecting labels...\n");

    asm_ctx.current_line = 0;
    while (LIKELY(fgets(line, sizeof(line), input))) {
        asm_ctx.current_line++;
        trim_line(line);
        strncpy(asm_ctx.current_source, line, sizeof(asm_ctx.current_source) - 1);
        asm_ctx.current_source[sizeof(asm_ctx.current_source) - 1] = '\0';

        if (UNLIKELY(strncmp(line, ".data", 5) == 0) ||
            UNLIKELY(strncmp(line, ".text", 5) == 0) ||
            UNLIKELY(asm_ctx.in_data_section)) {
            parse_data_directive(&asm_ctx, &err_ctx, line);
            continue;
        }

        parse_instruction(&asm_ctx, &err_ctx, line, 1);
    }

    /* recalculate .data label addresses after pass 1 */
    asm_ctx.data_start_addr = asm_ctx.bytecode_pos;
    for (int i = 0; i < asm_ctx.label_count; i++) {
        if (UNLIKELY(asm_ctx.labels[i].is_data)) {
            int offset = asm_ctx.labels[i].address - 0x0100;
            asm_ctx.labels[i].address = asm_ctx.data_start_addr + offset;
        }
    }

    /* ---- pass 2: emit bytecode ---- */
    if (LIKELY(!input_args.silent))
        printf("Second pass - generating code...\n");

    rewind(input);
    asm_ctx.bytecode_pos = 0;
    asm_ctx.in_data_section = 0;
    asm_ctx.current_line = 0;

    while (LIKELY(fgets(line, sizeof(line), input))) {
        asm_ctx.current_line++;
        trim_line(line);
        strncpy(asm_ctx.current_source, line, sizeof(asm_ctx.current_source) - 1);
        asm_ctx.current_source[sizeof(asm_ctx.current_source) - 1] = '\0';

        if (UNLIKELY(strncmp(line, ".data", 5) == 0) ||
            UNLIKELY(strncmp(line, ".text", 5) == 0)) {
            asm_ctx.in_data_section = (strncmp(line, ".data", 5) == 0);
            continue;
        }

        if (UNLIKELY(asm_ctx.in_data_section)) continue;

        parse_instruction(&asm_ctx, &err_ctx, line, 2);
    }

    fclose(input);

    /* disassemble mode (-v): skip writing binary */
    if (input_args.disass) {
        disass_vasm(&asm_ctx);
        return 0;
    }

    if (UNLIKELY(error_has_errors(&err_ctx))) {
        error_dump(&err_ctx);
        return 1;
    }

    if (UNLIKELY(err_ctx.warning_count > 0))
        error_dump(&err_ctx);

    /* write output */
    FILE *output = fopen(argv[2], "wb");
    if (UNLIKELY(!output)) {
        error_push(&err_ctx, ERR_FILE_OPEN, SEVERITY_FATAL, 0, 0, NULL,
                   "could not create file '%s'", argv[2]);
        return 1;
    }

    size_t bytes_written = fwrite(asm_ctx.bytecode, 1, asm_ctx.bytecode_pos, output);
    if (LIKELY(asm_ctx.data_pos > 0))
        bytes_written += fwrite(asm_ctx.data_section, 1, asm_ctx.data_pos, output);
    fclose(output);

    int total_bytes = asm_ctx.bytecode_pos + asm_ctx.data_pos;
    if (UNLIKELY(bytes_written != (size_t)total_bytes)) {
        error_push(&err_ctx, ERR_FILE_WRITE, SEVERITY_FATAL, 0, 0, NULL,
                   "incomplete write to '%s' (%zu of %d bytes written)",
                   argv[2], bytes_written, total_bytes);
        return 1;
    }

    if (LIKELY(!input_args.silent))
        printf(COLOR_GREEN "OK" COLOR_RESET " - %d bytes code, %d bytes data -> %s\n",
               asm_ctx.bytecode_pos, asm_ctx.data_pos, argv[2]);

    if (UNLIKELY(input_args.dump_labels)) dump_labels(&asm_ctx);
    if (UNLIKELY(input_args.debug_mode)) debug_hex(&asm_ctx);
    if (UNLIKELY(input_args.dump_data)) dump_data_section(&asm_ctx);

    return 0;
}