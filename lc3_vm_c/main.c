#include "vm.h"
#include <assert.h>

//101
int main(int argc, const char *argv[]) {
    if (argc < 2){
        printf("Usage: ./<name_of_program> <path_to_bin>");
        exit(1);
    }
    setup();
    readImageFile(argv[1]);
    emulate();
}
