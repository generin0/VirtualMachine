# VASM — Custom Virtual Machine & Assembler

A bytecode virtual machine with its own assembly language and two-pass assembler, written in C.

---

## Overview

The project consists of two independent components:

- **VM** - a register-based virtual machine that executes compiled bytecode
- **vasm** - an assembler that compiles `.vasm` source files into binary bytecode for the VM

---

## Project Structure

```
asm/
├── examples/                  - example .vasm programs
├── README.md
└── vasm_compiler/             - assembler source
    ├── include/
    │   ├── common.h           - limits, colors, optimization macros
    │   ├── opcodes.h          - Opcode enum
    │   ├── types.h            - all structs (Error, Assembler, InputArguments...)
    │   ├── error.h            - error handler declarations
    │   ├── assembler.h        - parser and emitter declarations
    │   ├── disasm.h           - disassembler declaration
    │   └── dump.h             - debug dump declarations
    ├── src/
    │   ├── main.c             - entry point, two-pass driver
    │   ├── error.c            - error context, push/dump logic
    │   ├── assembler.c        - instruction parser, byte emitter, .data section
    │   ├── disasm.c           - disassembler (-v)
    │   └── dump.c             - hex/label/data dump utilities
    └── Makefile

vm/
├── headers/
│   └── vm.h                   - VM types, constants, function declarations
├── src/
│   ├── core/                  - VM initialization, memory loading
│   ├── debug/                 - debug dump (registers, stack, memory near PC)
│   ├── flags/                 - CPU flags logic (zero, sign, carry, overflow)
│   └── opcodes/               - instruction execution (fetch-decode-execute loop)
├── tests/                     - tests, the same as in examples/
├── main.c                     - entry point
└── makefile
```

---

## Virtual Machine

### Architecture

| Parameter     | Value                          |
|---------------|--------------------------------|
| Memory        | 1024 bytes (`0x0000`–`0x03FF`) |
| Registers     | 8 × 32-bit (`R0`–`R7`)         |
| Stack         | 64 × 32-bit integers           |
| PC            | 16-bit program counter         |
| Flags         | Zero, Sign, Carry, Overflow    |

### Flags

Flags are updated automatically after arithmetic and comparison instructions:

- **Zero (ZF)** — result is zero
- **Sign (SF)** — result is negative
- **Carry (CF)** — unsigned overflow on ADD / borrow on SUB
- **Overflow (OF)** — signed overflow on ADD / SUB

### Instruction Set

All instructions are encoded as a sequence of bytes. The first byte is the opcode, followed by operand bytes.

#### Arithmetic

| Instruction         | Encoding          | Description                                       |
|---------------------|-------------------|---------------------------------------------------|
| `ADD Rd, Rs1, Rs2`  | `01 Rd Rs1 Rs2`   | `Rd = Rs1 + Rs2`                                  |
| `ADDI Rd, Rs, imm`  | `02 Rd Rs imm`    | `Rd = Rs + imm`                                   |
| `SUB Rd, Rs1, Rs2`  | `03 Rd Rs1 Rs2`   | `Rd = Rs1 - Rs2`                                  |
| `MUL Rd, Rs1, Rs2`  | `04 Rd Rs1 Rs2`   | `Rd = Rs1 * Rs2` (signed 64-bit, truncated to 32) |
| `DIV Rd, Rs1, Rs2`  | `05 Rd Rs1 Rs2`   | `Rd = Rs1 / Rs2` (signed; halts on div by zero)   |

#### Data Movement

| Instruction        | Encoding          | Description                               |
|--------------------|-------------------|-------------------------------------------|
| `MOV Rd, Rs`       | `06 Rd Rs`        | `Rd = Rs`                                 |
| `LOAD Rd, hi, lo`  | `0E Rd hi lo`     | `Rd = (hi << 8) \| lo` (16-bit immediate) |
| `STORE Rd, Ra, Rb` | `15 Rd Ra Rb`     | `memory[Ra] = Rd` (32-bit, big-endian)    |
| `STOREI Rd, imm`   | `18 Rd imm`       | `memory[imm] = Rd` (32-bit, big-endian)   |
| `LDB Rd, Ra`       | `22 Rd Ra`        | `Rd = memory[Ra]` (single byte)           |

#### Bitwise & Shifts

| Instruction        | Encoding         | Description         |
|--------------------|------------------|---------------------|
| `AND Rd, Rs1, Rs2` | `25 Rd Rs1 Rs2`  | `Rd = Rs1 & Rs2`    |
| `OR Rd, Rs1, Rs2`  | `26 Rd Rs1 Rs2`  | `Rd = Rs1 \| Rs2`   |
| `ORI Rd, Rs, imm`  | `27 Rd Rs imm`   | `Rd = Rs \| imm`    |
| `XOR Rd, Rs1, Rs2` | `0F Rd Rs1 Rs2`  | `Rd = Rs1 ^ Rs2`    |
| `XORI Rd, Rs, imm` | `10 Rd Rs imm`   | `Rd = Rs ^ imm`     |
| `SHL Rd, Rs1, Rs2` | `11 Rd Rs1 Rs2`  | `Rd = Rs1 << Rs2`   |
| `SHLI Rd, Rs, imm` | `12 Rd Rs imm`   | `Rd = Rs << imm`    |
| `SHR Rd, Rs1, Rs2` | `13 Rd Rs1 Rs2`  | `Rd = Rs1 >> Rs2`   |
| `SHRI Rd, Rs, imm` | `14 Rd Rs imm`   | `Rd = Rs >> imm`    |

#### Comparison

| Instruction     | Encoding      | Description                 |
|-----------------|---------------|-----------------------------|
| `CMP Rs1, Rs2`  | `07 Rs1 Rs2`  | sets flags from `Rs1 - Rs2` |
| `CMPI Rs, imm`  | `24 Rs imm`   | sets flags from `Rs - imm`  |

#### Jumps & Calls

Jump targets are **2-byte absolute addresses** (`hi lo`), allowing the full 16-bit address space.

| Instruction  | Encoding          | Condition                   |
|--------------|-------------------|-----------------------------|
| `JMP addr`   | `08 hi lo`        | unconditional               |
| `JE addr`    | `09 hi lo`        | ZF = 1                      |
| `JG addr`    | `0A hi lo`        | ZF = 0 and SF = OF          |
| `JNZ addr`   | `0B hi lo`        | ZF = 0                      |
| `JL addr`    | `1E hi lo`        | SF ≠ OF                     |
| `JLE addr`   | `1F hi lo`        | ZF = 1 or SF ≠ OF           |
| `JGE addr`   | `20 hi lo`        | SF = OF                     |
| `JNE addr`   | `21 hi lo`        | ZF = 0                      |
| `CALL addr`  | `16 hi lo`        | push PC, jump to addr       |
| `RET`        | `17`              | pop PC from stack           |

#### Stack

| Instruction | Encoding  | Description               |
|-------------|-----------|---------------------------|
| `PUSH Rs`   | `0C Rs`   | push register onto stack  |
| `POP Rd`    | `0D Rd`   | pop top of stack to Rd    |

#### I/O

| Instruction            | Encoding          | Description                                    |
|------------------------|-------------------|------------------------------------------------|
| `PRINT Rs`             | `19 Rs`           | print register value as integer                |
| `PRINTC Rs`            | `1A Rs`           | print register value as ASCII character        |
| `PRINTS Rs`            | `23 Rs`           | print null-terminated string at address in Rs  |
| `READ Rd`              | `1B Rd`           | read integer from stdin into Rd                |
| `READC Rd`             | `1C Rd`           | read single character from stdin into Rd       |
| `READS addr, maxlen`   | `1D addr maxlen`  | read string from stdin into memory[addr]       |

#### Misc

| Instruction | Opcode | Description                                        |
|-------------|--------|----------------------------------------------------|
| `NOP`       | `60`   | no operation                                       |
| `HALT`      | `00`   | stop execution                                     |
| `DBG`       | `FF`   | dump registers, stack, and memory to stdout        |

---

## Assembler (vasm)

### Usage

```
./vasm_compiler <input.vasm> <output.bin> <-flags>
```

**Flags:**

| Flag              | Description                                |
|-------------------|--------------------------------------------|
| `-v`, `--vasm`    | disassemble input file (no output written) |
| `-d`, `--debug`   | print generated bytecode in hex            |
| `-l`, `--labels`  | print collected labels and addresses       |
| `-D`, `--data`    | dump `.data` section contents              |
| `-s`, `--silent`  | suppress compilation output                |
| `-h`, `--help`    | show help                                  |

### Assembly Syntax

Comments start with `;`. Labels end with `:`. Mnemonics are case-insensitive.

```asm
; example: print numbers 1 to 5
    LOAD R0, 0, 1     ; R0 = 1
    LOAD R1, 0, 5     ; R1 = 5 (limit)
    LOAD R2, 0, 1     ; R2 = 1 (step)
loop:
    PRINT R0
    LOAD R3, 0, 10
    PRINTC R3         ; newline
    ADD R0, R0, R2
    CMP R0, R1
    JLE loop
    HALT
```

### Data Section

Strings and byte arrays can be defined in a `.data` section and referenced by label:

```asm
.data
    msg:   "Hello, world!\n"
    table: 1, 2, 3, 4

.text
    LOAD R0, msg      ; R0 = address of msg
    PRINTS R0
    HALT
```

String literals support escape sequences: `\n`, `\t`, `\r`, `\\`, `\"`, `\0`. Strings are automatically null-terminated.

### Numbers

Immediate values support decimal, hexadecimal (`0x...`), and binary (`0b...`) notation.

```asm
LOAD R0, 0, 42        ; decimal
LOAD R1, 0, 0xFF      ; hexadecimal
ADDI R2, R2, 0b1010   ; binary
```

### Two-Pass Compilation

The assembler works in two passes:

1. **Pass 1** — scan the source, collect all labels and their byte offsets, resolve `.data` section addresses
2. **Pass 2** — emit bytecode with all label references resolved from the table built in pass 1

Errors during pass 2 are accumulated and printed together — the assembler does not stop at the first error.

### Error Handling

Errors are categorized by severity:

| Severity | Description |
|----------|-------------|
| `WARN`   | non-fatal; compilation continues (e.g. duplicate label, consecutive NOPs) |
| `ERROR`  | invalid instruction or operand; bytecode is not written |
| `FATAL`  | unrecoverable (buffer overflow, file not found); exits immediately |

All messages include source line number and the offending source line.

---

## Building

### VM

```bash
gcc -O2 -o vm main.c vm_core.c vm_opcodes.c vm_flags.c vm_dbg.c
```

### Assembler

```bash
cd vasm_compiler
make
```

On Windows (MSYS2 / MinGW64):
```bash
pacman -S mingw-w64-x86_64-gcc make
cd vasm_compiler
make
```

Output binary: `vasm_compiler.exe`

---

## Running

```bash
# compile a source file
./vasm_compiler program.vasm program.bin

# disassemble without compiling
./vasm_compiler program.vasm -v

# run on the VM
./vm program.bin
```

---

## Limitations

- Memory is flat, 1024 bytes total — code and data share the same address space
- Jump addresses are 16-bit (2 bytes), supporting the full 1024-byte memory range
- `LOAD` accepts a 16-bit immediate split across two bytes (`hi`, `lo`)
- Stack depth is fixed at 64 entries; overflow halts the VM
- Immediate values for arithmetic/logic instructions are 8-bit (`-128`..`255`)
