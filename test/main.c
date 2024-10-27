#include <stdio.h>
#include "window.h"


int test_window() {
    fprintf(stdout, "[Test]: Runing test: window\n");
    int status = create_window();
    if (!status) {
        fprintf(stdout, "[Test]: Test was successful\n");
        return 0;
    }
    else fprintf(stdout, "[Test]: Test failed with error code %d\n", status);
    return 1;
}

int main(int argc, char* argv[]) {
    test_window();
} 

