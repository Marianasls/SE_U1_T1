#include "led_matrix.h"

bool led_buffer[NUM_PIXELS] = { 0 };

// Matriz de LEDs
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// recebe matrizes de índices e cores
void set_leds(int indices[][NUM_PIXELS], uint8_t cores[][3], int num_cores) {
    uint32_t pixel_colors[NUM_PIXELS] = {0}; // buffer de cores de cada LED
    
    // Para cada grupo de cor
    for (int cor = 0; cor < num_cores; cor++) {
        uint8_t r = cores[cor][0];
        uint8_t g = cores[cor][1];
        uint8_t b = cores[cor][2];
        uint32_t color = urgb_u32(r, g, b);

        // Atribuir cor aos LEDs indicados
        for (int i = 0; i < NUM_PIXELS; i++) {
            int idx = indices[cor][i];
            if (idx >= 0 && idx < NUM_PIXELS) { // checar se o índice é válido
                pixel_colors[idx] = color;
            }
        }
    }

    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(pixel_colors[i]);
    }
}

void clear_buffer() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        led_buffer[i] = 0;
    }
}
