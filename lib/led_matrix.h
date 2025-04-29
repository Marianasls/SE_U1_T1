#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "hardware/pio.h"

#define NUM_PIXELS 25

static inline void put_pixel(uint32_t pixel_grb);

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);

void set_leds(int indices[][NUM_PIXELS], uint8_t cores[][3], int num_cores);

void clear_buffer();

void turn_on_leds(int index[], int size);

#endif
