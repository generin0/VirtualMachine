CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iheaders -Wno-type-limits
TARGET = vm.exe
SOURCES = main.c src/core/vm_core.c src/debug/vm_dbg.c src/flags/vm_flags.c src/opcodes/vm_opcodes.c

all: $(TARGET)

$(TARGET): $(SOURCES) headers/vm.h headers/vm_tests.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)
	@echo ========================================
	@echo   Build successful: $(TARGET)
	@echo ========================================

clean:
	@echo Cleaning build files...
	-@del $(TARGET) 2>nul || echo.
	-@del *.o 2>nul || echo.
	-@del src\core\*.o 2>nul || echo.
	-@del src\debug\*.o 2>nul || echo.
	-@del src\flags\*.o 2>nul || echo.
	-@del src\opcodes\*.o 2>nul || echo.
	@echo Clean completed

run: $(TARGET)
	@echo ========================================
	@echo   Running Virtual Machine
	@echo ========================================
	@$(TARGET)

rebuild: clean all run

debug: CFLAGS += -g -O0
debug: clean $(TARGET)
	@echo Debug build completed

quick:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)
	@echo Quick build done

help:
	@echo ========================================
	@echo   Virtual Machine Build System
	@echo ========================================
	@echo Commands:
	@echo   make         - Build vm.exe
	@echo   make clean   - Remove all build files
	@echo   make run     - Build and run
	@echo   make rebuild - Clean, build and run
	@echo   make debug   - Build with debug info
	@echo   make quick   - Fast compile (no checks)
	@echo   make help    - Show this help
	@echo.
	@echo Structure:
	@echo   headers/     - Header files (.h)
	@echo   src/core/    - Core VM functions
	@echo   src/debug/   - Debug utilities
	@echo   src/flags/   - Flag management
	@echo   src/opcodes/ - Instruction handlers

.PHONY: all clean run rebuild debug quick help