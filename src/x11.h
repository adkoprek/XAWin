#ifndef X11_H
#define X11_H
#include <stdint.h>

typedef struct {
    uint16_t protocol_major_version;
    uint16_t protocol_minor_version;
    uint16_t additional_data_lenght;
    uint32_t release_number;
    uint32_t resource_id_base;
    uint32_t resource_id_mask;
    uint32_t motion_buffer_size;
    uint16_t vendor_lenght;
    uint16_t max_request_lenght;
    uint8_t  number_of_screens;
    uint8_t  number_pixmap_formats;
    uint8_t  image_byte_order;

    
    uint32_t global_id;
    uint32_t global_id_base;
    uint32_t global_id_mask;
    uint32_t root_window;
    uint32_t root_visual_id;
} x11_details;

typedef struct {
    uint8_t byte_order; 
    uint8_t padding1;
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t auth_name_len;
    uint16_t auth_data_len;
    uint16_t padding2;
} init_request_structure;

typedef struct {
    uint8_t request_id;
    uint8_t depth;
    uint16_t request_lenght;
    uint32_t window_id;
    uint32_t root_window;
    uint16_t x_position;
    uint16_t y_position;
    uint16_t width;
    uint16_t height;
    uint16_t border_width;
    uint16_t window_class;
    uint32_t root_visual_id;
    uint32_t events;
    uint32_t no_idea;
    uint32_t additional_events;
} init_window_request;

typedef struct {
    uint8_t request_id;
    uint8_t padding1;
    uint16_t lenght;
    uint32_t window_id;
} map_window_request;

#endif
