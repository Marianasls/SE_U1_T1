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
ssd1306_t ssd;              // Estrutura para o display OLED

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
void init_all_pins() {
    stdio_init_all();
    
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

    // Inicializa o display OLED
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
}

int main() {
    init_all_pins();

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
        printf("Resistência E24 mais próxima: %d\n", resistencia_e24);

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
            leitura_anterior = resistencia_e24;
        
            uint32_t cores_leds[] = {faixa1, faixa2, multiplicador};
            uint8_t r, g, b, intensidade = 20;
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
                set_leds(r, g, b);
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
    // Calcular o multiplicador (potência de 10)
    int potencia = 0;
    int valor_original = resistencia;
    
    // Reduzir o valor para obter os primeiros dois dígitos significativos
    while (resistencia >= 100) {
        resistencia /= 10;
        potencia++;
    }
    
    // Extrair os dígitos
    *faixa1 = resistencia / 10;
    *faixa2 = resistencia % 10;
    *multiplicador = potencia;
    
    // Tratar casos especiais (< 10 ohms)
    if (*faixa1 == 0) {
        *faixa1 = *faixa2;
        *faixa2 = 0;
        if (potencia > 0) {
            // Ajustar o multiplicador se houve deslocamento
            (*multiplicador)--;
        }
    }
    
    printf("Valor original: %d -> faixa1: %d, faixa2: %d, multiplicador: %d\n", valor_original, *faixa1, *faixa2, *multiplicador);
}
