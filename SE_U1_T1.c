/*
 * Autor: Mariana da Silva Lima Santos
 * Ohmímetro utilizando o ADC da BitDogLab
 *
 *
 * O projeto tem o objetivo de medir a resistência de um resistor desconhecido, 
 * utilizando um divisor de tensão com dois resistores e ADC do RP2040 para fazer a leitura.
 * O resistor conhecido é de 10k ohm e o desconhecido é o que queremos medir.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/led_matrix.h"
#include "lib/ws2812.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o ohmimetro
#define Botao_A 5  // GPIO para botão A
#define NUM_PIXELS 25
#define WS2812_PIN 7
// desconhecido: 11.76 kohm
int R_conhecido = 10000;   // Resistor de 10k ohm
float R_x = 0.0;           // Resistor desconhecido
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

// Valores da série E24 (5% de tolerância)
const int E24[] = {
    10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30, 33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91
};
// Valores da série E96 (1% de tolerância)
const float E96 = {
    10.0, 10.2, 10.5, 10.7, 11.0, 11.3, 11.5, 11.8,
    12.1, 12.4, 12.7, 13.0, 13.3, 13.7, 14.0, 14.3,
    14.7, 15.0, 15.4, 15.8, 16.2, 16.5, 16.9, 17.4,
    17.8, 18.2, 18.7, 19.1, 19.6, 20.0, 20.5, 21.0,
    21.5, 22.1, 22.6, 23.2, 23.7, 24.3, 24.9, 25.5,
    26.1, 26.7, 27.4, 28.0, 28.7, 29.4, 30.1, 30.9,
    31.6, 32.4, 33.2, 34.0, 34.8, 35.7, 36.5, 37.4,
    38.3, 39.2, 40.2, 41.2, 42.2, 43.2, 44.2, 45.3,
    46.4, 47.5, 48.7, 49.9, 51.1, 52.3, 53.6, 54.9,
    56.2, 57.6, 59.0, 60.4, 61.9, 63.4, 64.9, 66.5,
    68.1, 69.8, 71.5, 73.2, 75.0, 76.8, 78.7, 80.6,
    82.5, 84.5, 86.6, 88.7, 90.9, 93.1, 95.3, 97.6
};

// Cores correspondentes às faixas
const char *cores[] = {
    "black",    // digito 0
    "brown",   // digito 1
    "red",     // digito 2
    "orange",  // digito 3
    "yellow",  // digito 4
    "green",    // digito 5
    "blue",     // digito 6
    "lilac",  // digito 7
    "gray",    // digito 8
    "white"    // digito 9
};
// rgb equivalente para cada cor
const uint8_t cores_rgb[][3] = {
    {0, 0, 0},       // Preto
    {139, 69, 19},   // Marrom
    {255, 0, 0},     // Vermelho
    {255, 165, 0},   // Laranja
    {255, 255, 0},   // Amarelo
    {0, 255, 0},     // Verde
    {0, 0, 255},     // Azul
    {138, 43, 226},  // Violeta
    {128, 128, 128}, // Cinza
    {255, 255, 255}  // Branco
};

// declarando as funções
void determine_colors(int resistencia, int *faixa1, int *faixa2, int *multiplicador);
int find_E24_value(float resistencia);

int main() {
    // modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_t ssd;                                                // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicializar a matriz de LEDs (WS2812)
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, false);

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

    float tensao;
    char str_x[5];                                           // Buffer para armazenar a string
    char str_y[5];                                           // Buffer para armazenar a string
    char str_colors[20], color1[10], color2[10], color3[10]; // Buffer para armazenar a string

    bool cor = true;
    int faixa1, faixa2, multiplicador, leitura_anterior = 0;
    while (true) {   
        adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

        // realiza a media com 500 leituras do ADC para evitar ruídos
        float soma = 0.0f;
        for (int i = 0; i < 500; i++) {
            soma += adc_read();
            sleep_ms(1);
        }
        float media = soma / 500.0f;

        // Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
        R_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
        printf("R_x: %.2f\n", R_x);                   // Imprime o valor de R_x
        tensao = (ADC_VREF * media) / ADC_RESOLUTION; // Converte o valor do ADC para tensão
        printf("Tensão: %.2f\n", tensao);             // Imprime a tensão

        // Encontrar o valor E24 mais próximo
        int resistencia_e24 = find_E24_value(R_x);

        // Determinar as cores das faixas
        determine_colors(resistencia_e24, &faixa1, &faixa2, &multiplicador);
        printf("Faixa 1: %d\n", faixa1);
        printf("Faixa 2: %d\n", faixa2);
        printf("Multiplicador: %d\n", multiplicador);

        sprintf(str_x, "%1.0f", media);              // Converte o inteiro em string
        sprintf(str_y, "%1.f", R_x);                 // Converte o float em string
        sprintf(color1, "%s", cores[faixa1]);        // Converte o inteiro em string
        sprintf(color2, "%s", cores[faixa2]);
        sprintf(color3, "%s", cores[multiplicador]);

        ssd1306_fill(&ssd, !cor);                     // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);   // Desenha uma linha
        ssd1306_draw_string(&ssd, "Faixa1", 8, 6);   // Desenha uma string
        ssd1306_draw_string(&ssd, color1, 60, 6);
        ssd1306_draw_string(&ssd, "Faixa2", 8, 16);
        ssd1306_draw_string(&ssd, color2, 60, 16);
        ssd1306_draw_string(&ssd, "Multi.", 8, 26);
        ssd1306_draw_string(&ssd, color3, 60, 26);
        ssd1306_draw_string(&ssd, "ADC", 13, 41);
        ssd1306_draw_string(&ssd, "Resisten.", 52, 41);
        ssd1306_line(&ssd, 48, 37, 48, 60, cor);        // Desenha uma linha vertical
        ssd1306_draw_string(&ssd, str_x, 8, 52);
        ssd1306_draw_string(&ssd, str_y, 59, 52);
        ssd1306_send_data(&ssd);                        // Atualiza o display

        // Atualizar a matriz de LEDs para exibir as cores
        if (leitura_anterior != resistencia_e24) {
            // Limpa a matriz de LEDs
            clear_buffer();
            set_leds(0, 0, 0); // Limpa a matriz de LEDs 
            leitura_anterior = resistencia_e24;
        
            uint32_t cores_leds[] = {faixa1, faixa2, multiplicador};
            uint8_t r, g, b, intensidade = 255;
            for (int i = 0; i < 3; i++) {
                r = cores_rgb[cores_leds[i]][0];
                g = cores_rgb[cores_leds[i]][1];
                b = cores_rgb[cores_leds[i]][2];

                // Aplicar intensidade
                r = (r * intensidade) / 255;
                g = (g * intensidade) / 255;
                b = (b * intensidade) / 255;

                uint32_t cor = cores_leds[i];
                printf("Cor: %s\n", cores[cor]);

                int indices[] = {1+(i*10), 2+(i*10), 3+(i*10)};

                turn_on_leds(indices, 3);
                set_leds(cores_rgb[cor][0], cores_rgb[cor][1], cores_rgb[cor][2]);
            }
        }
        
        sleep_ms(700); // espera 700ms até fazer a próxima leitura
    }
}

// Função para encontrar o valor E24 mais próximo
int find_E24_value(float resistencia) {
    int multiplicador = 1;
    while (resistencia > 100) {
        resistencia /= 10;
        multiplicador *= 10;
    }
    int valor_proximo = E24[0];
    for (int i = 1; i < sizeof(E24) / sizeof(E24[0]); i++) {
        if (fabs(resistencia - E24[i]) < fabs(resistencia - valor_proximo)) {
            valor_proximo = E24[i];
        }
    }
    return valor_proximo * multiplicador;
}

// Função para determinar as cores das faixas
void determine_colors(int resistencia, int *faixa1, int *faixa2, int *multiplicador) {
    int digitos = resistencia;
    *faixa1 = digitos / 10; // Primeiro dígito
    *faixa2 = digitos % 10; // Segundo dígito
    *multiplicador = log10(resistencia / (*faixa1 * 10 + *faixa2));
}
