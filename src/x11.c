#include "x11.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include "globals.h"


int win_socket = -1;
x11_details details = { 0 };

static char send_buffer[BUFFER_SIZE] = {};

static void handle_x11_error(char* read_buffer) {
    uint8_t reason_len = (uint8_t)read_buffer[1];
    uint16_t major_version = *((uint16_t*)&read_buffer[2]);
    uint16_t minor_version = *((uint16_t*)&read_buffer[4]);
    uint8_t* message = (uint8_t*)&read_buffer[8];
    
    read(win_socket, read_buffer + 8, BUFFER_SIZE - 8);

    fprintf(stderr, "[XAWin]: X11 returned an error\n");
    fprintf(stderr, "   - Major Version: %d\n", major_version);
    fprintf(stderr, "   - Minor Version: %d\n", minor_version);
    fprintf(stderr, "   - The reason:    %s", message);
    if (message[reason_len - 1] != '\n') fprintf(stderr, "\n");
}

static void handle_x11_init_response(char* read_buffer) {
    read(win_socket, read_buffer + 8, BUFFER_SIZE - 8); 

    uint32_t resource_id_base = *((uint32_t*)&read_buffer[12]);
    uint32_t resource_id_mask = *((uint32_t*)&read_buffer[16]);
    uint16_t lenght_of_vendor = *((uint16_t*)&read_buffer[24]);
    uint8_t number_of_formats = *((uint16_t*)&read_buffer[29]);
    uint8_t* vendor = (uint8_t*)&read_buffer[40];

    int32_t vendor_pad = PAD(lenght_of_vendor);
    int32_t format_byte_lenght = 8 * number_of_formats;
    int32_t screen_start_offset = 40 + lenght_of_vendor + vendor_pad + format_byte_lenght;

    uint32_t root_window =    *((uint32_t*)&read_buffer[screen_start_offset]);
    uint32_t root_visual_id = *((uint32_t*)&read_buffer[screen_start_offset + 32]);

    details.global_id = 0;
    details.global_id_base = resource_id_base;
    details.global_id_mask = resource_id_mask;
    details.root_window = root_window;
    details.root_visual_id = root_visual_id;
}

static char* extract_display_number(const char* display_env, size_t* display_size) {
    size_t display_env_len = strlen(display_env);
    size_t hostname_offset = 1;
    static char output[5];
    *display_size = 0;

    for (size_t i = 0; i < display_env_len; i++)
        if (display_env[i] == ':') hostname_offset = i + 1;

    while (((*display_size) + hostname_offset) < display_env_len) {
        if (output[hostname_offset + (*display_size)] == '.') break;
        output[*display_size] = display_env[hostname_offset + (*display_size)];
        (*display_size)++;
    }
    output[(*display_size)++] = '\0';

    return output;
}

static init_request_structure* get_authentication(size_t* lenght) {
    const char* hostname = getenv("HOSTNAME");
    if (hostname == NULL) {
        fprintf(stdout, "[XAWin]: Could not get the value of $HOSTNAME, defaulting to localhost");
        hostname = "localhost";
    }

    const char* display = getenv("DISPLAY");
    if (display == NULL) {
        fprintf(stderr, "[XAWin]: An error occured while trying to find out the X-Display");
        return NULL;
    }
    size_t display_num_len;
    char* display_num = extract_display_number(display, &display_num_len);

    const char* xauthority = getenv("XAUTHORITY");
    if (xauthority == NULL) xauthority = "~/.Xauthority";

    FILE* file = fopen(xauthority, "rb"); 
    if (file == NULL) {
        fprintf(stdout, "[XAWin]: Could not open the Xauthority file under the path: ");
        fprintf(stdout, "%s, assumin there is no authentication required\n", xauthority);
        *lenght = sizeof(init_request_structure);
        init_request_structure* init_reqeust = malloc(*lenght);
        init_reqeust->auth_name_len = 0;
        init_reqeust->auth_data_len = 0;
        return init_reqeust;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET); 

    char* file_buffer = malloc(size);
    fread(file_buffer, 1, size, file);

    size_t ptr = 0;
    while (ptr < size) {
        ptr += 2;

        uint16_t address_len = (uint16_t)(file_buffer[ptr] << 8) | file_buffer[ptr + 1];
        ptr += 2;
        char address[address_len + 1];
        memcpy(address, file_buffer + ptr, address_len);
        address[address_len] = '\0';
        ptr += address_len;

        size_t display_len = (uint16_t)(file_buffer[ptr] << 8) | file_buffer[ptr + 1]; 
        ptr += 2;
        char display[display_len + 1];
        memcpy(display, file_buffer + ptr, display_len);
        display[display_len] = '\0';
        ptr += display_len;

        if (!strcmp(address, hostname) && (!strcmp(display, display_num) || display_len == 0)) {
            uint16_t auth_name_len = (uint16_t)(file_buffer[ptr] << 8) | file_buffer[ptr + 1];
            ptr += 2;
            char auth_name[auth_name_len];
            auth_name[auth_name_len] = '\0';
            memcpy(auth_name, file_buffer + ptr, auth_name_len);
            ptr += auth_name_len;

            uint16_t auth_data_len = (uint16_t)(file_buffer[ptr] << 8) | file_buffer[ptr + 1];
            ptr += 2;
            char auth_data[auth_name_len];
            auth_data[auth_data_len] = '\0';
            memcpy(auth_data, file_buffer + ptr, auth_data_len);
            ptr += auth_data_len;

            *lenght = sizeof(init_request_structure) + auth_name_len + auth_data_len + 2;
            init_request_structure* init_reqeust = malloc(*lenght);
            init_reqeust->auth_name_len = auth_name_len;
            init_reqeust->auth_data_len = auth_data_len;
            memcpy(((char*)init_reqeust) + sizeof(init_request_structure), auth_name, auth_name_len);
            memcpy(((char*)init_reqeust) + sizeof(init_request_structure) + auth_name_len, PADDING_2, 2);
            memcpy(((char*)init_reqeust) + sizeof(init_request_structure) + auth_name_len + 2, auth_data, auth_data_len);

            free(file_buffer);
            fclose(file);

            return init_reqeust;
        }
        else while (TRUE && (ptr < size)) if (file_buffer[ptr++] == '\n') break;
    }

    free(file_buffer);
    fclose(file);
    return NULL;
}

static int get_next_x11_id() {
    return (details.global_id_mask & details.global_id++) | details.global_id_base;
}


int init_socket() {
    win_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (win_socket < 0) {
        fprintf(stderr, "[Window-Linux]: Error while createating a socket\n");
        return -1;
    }        

    return 0;
}

int connect_to_x11() {
    struct sockaddr_un address; 
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, "/tmp/.X11-unix/X0", sizeof(address.sun_path) - 1);

    int status = connect(win_socket, (struct sockaddr*)&address, sizeof(address));
    if (status < 0) {
        fprintf(stderr, "[XAWin]: Error while trying to connect to the X-Server\n");
        return -1;
    }

    return 0;
}

int initialize_x11(int authenticate) {
    char read_buffer[BUFFER_SIZE] = {};

    size_t request_len;
    init_request_structure* init_request = get_authentication(&request_len);
    init_request->byte_order = 0154;
    init_request->padding1 = 0x00;
    init_request->major_version = 0x000B;
    init_request->minor_version = 0x0000;
    init_request->padding2 = 0x0000;

    int bytes_written = write(win_socket, (char*)init_request, request_len);
    if (bytes_written != request_len) {
        fprintf(stderr, "[XAWin]: Error while sending initialization request\n");
        return -1;
    }

    free(init_request);

    read(win_socket, read_buffer, 8);
    switch (read_buffer[0]) {
        case RESPONSE_STATE_FAILED:
            handle_x11_error(read_buffer);
            break;

        case RESPONSE_STATE_AUTHENTICATE:
            fprintf(stderr, "[XAWin]: Authentication required\n");
            break;

        case RESPONSE_STATE_SUCCESS:
            handle_x11_init_response(read_buffer);
            return 0;
    }

    return -1;
}

int create_x11_window() {
    int32_t window_id = get_next_x11_id();

    init_window_request* init_request = malloc(sizeof(init_window_request));
    init_request->request_id = X11_REQUEST_CREATE_WINDOW;
    init_request->depth = 0;
    init_request->request_lenght = sizeof(init_window_request) / 4;
    init_request->window_id = window_id;
    init_request->root_window = details.root_window;
    init_request->x_position = 100;
    init_request->y_position = 100;
    init_request->width = 600;
    init_request->height = 300;
    init_request->border_width = 1;
    init_request->window_class = WINDOWCLASS_INPUTOUTPUT;
    init_request->root_visual_id = details.root_visual_id;
    init_request->events = X11_FLAG_WIN_EVENT | X11_FLAG_BACKGROUND_PIXEL;
    init_request->no_idea = 0xff000000;
    init_request->additional_events = X11_EVENT_FLAG_EXPOSURE | X11_EVENT_FLAG_KEY_PRESS;

    write(win_socket, (char*)init_request, sizeof(init_window_request));
    free(init_request);
    
    map_window_request* map_request = malloc(sizeof(map_window_request));
    map_request->request_id = X11_REQUEST_MAP_WINDOW;
    map_request->padding1 = 0;
    map_request->lenght = 2;
    map_request->window_id = window_id;
    write(win_socket, (char*)map_request, sizeof(map_window_request));
    free(map_request);

    struct pollfd poll_descriptors[1] = {};
    poll_descriptors[0].fd = win_socket;
    poll_descriptors[0].events = POLLIN;
    int32_t descriptor_count = 1;

    int32_t IsProgramRunning = 1;
    while(IsProgramRunning){
        int32_t EventCount = poll(poll_descriptors, descriptor_count, -1);

        if(poll_descriptors[0].revents & POLLERR) {
            fprintf(stderr, "------- Error\n");
        }

        if(poll_descriptors[0].revents & POLLHUP) {
            printf("---- Connection close\n");
            IsProgramRunning = 0;
        }

        char Buffer[1024] = {};
        int32_t BytesRead = read(poll_descriptors[0].fd, Buffer, 1024);
        uint8_t Code = Buffer[0];

        if (Code == 0 && Buffer[1]) {
            char ErrorCode = Buffer[1];
            printf("\033[0;31m");
            printf("Response Error: [%d]", ErrorCode);
            printf("\033[0m\n");
        } else if (Code == 1) {
            printf("---------------- Reply to request\n");
        } else {
            char EventCode = Buffer[0];
            printf("Some event occured: %d\n", EventCode);
        }
    }

    return 0;
}
