#include "window.h"


extern int init_socket();
extern int connect_to_x11();
extern int initialize_x11();
extern int create_x11_window();

int create_window() {
    if (init_socket()       != 0) return -1;
    if (connect_to_x11()    != 0) return -1;
    if (initialize_x11()    != 0) return -1;
    if (create_x11_window() != 0) return -1;

    return 0;
}

