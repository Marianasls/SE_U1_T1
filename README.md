# Projeto de Software Embarcado - Ohmímetro utilizando o ADC da BitDogLab

## Descrição do Projeto

Este projeto implementa um ohmímetro utilizando o microcontrolador RP2040 da Raspberry Pi Pico. O objetivo é medir a resistência de um resistor desconhecido em um divisor de tensão, utilizando o ADC (Conversor Analógico-Digital) do RP2040 para realizar as leituras.

O sistema também exibe as informações no display OLED e utiliza uma matriz de LEDs WS2812 para representar as cores das faixas do resistor medido.

## Funcionalidades

- Medição de resistores desconhecidos utilizando um divisor de tensão.
- Exibição dos valores medidos no display OLED.
- Representação das cores das faixas do resistor na matriz de LEDs WS2812.
- Suporte para valores de resistores das séries E24 (5% de tolerância).

## Componentes Utilizados

- **Microcontrolador**: Raspberry Pi Pico (RP2040).
- **Display OLED**: Controlado via protocolo I2C.
- **Matriz de LEDs WS2812**: Para exibição das cores das faixas do resistor.
- **Botões**: Para interação com o sistema.
- **Resistor conhecido**: 10kΩ.

## Configuração de Hardware

- **I2C SDA**: GPIO 14.
- **I2C SCL**: GPIO 15.
- **ADC**: GPIO 28.
- **Botão A**: GPIO 5.
- **Matriz WS2812**: GPIO 7.

## Como Funciona

1. O resistor desconhecido é conectado em um divisor de tensão com um resistor conhecido de 10kΩ.
2. O ADC do RP2040 lê a tensão no ponto médio do divisor.
3. A resistência do resistor desconhecido é calculada com base na fórmula do divisor de tensão.
4. O valor calculado é arredondado para o resistor mais próximo da série E24.
5. As cores das faixas do resistor são determinadas e exibidas no display OLED e na matriz de LEDs.

## Como Compilar e Executar

1. Certifique-se de que o SDK do Raspberry Pi Pico está configurado no seu ambiente.
2. Compile o projeto utilizando o CMake:
   ```sh
   mkdir build
   cd build
   cmake ..
   make
