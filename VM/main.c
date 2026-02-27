#include "F:\PY\VM\headers\vm.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    VM vm;
    vm_init(&vm);

    if (argc > 1) {
        vm_load_prog_input(&vm, argv[1]);
    } else {
        printf("No args were specified.");
        return 1;
    }
    
    int step_count = 0;
    while (vm.running) {
        vm_step(&vm);
        step_count++;
        
        if (step_count > 1000) {
            printf("\ninfinite loop.\n");
            break;
        }
    }
    
    printf("\nprogram completed in %d steps.\n", step_count);
    return 0;
}