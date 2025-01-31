#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define CONTADOR_LED 25
#define PINO_MATRIZ_LED 7
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define PINO_LED_VERMELHO 13
#define INTENSIDADE_COR_LED 200

// Definição de pixel GRB
struct pixel_t {
	uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t LED_da_matriz;

// Declaração do buffer de pixels que formam a matriz.
LED_da_matriz leds[CONTADOR_LED];

// Variáveis para uso da máquina PIO.
PIO maquina_pio;
uint variavel_maquina_de_estado;

// Variáveis globais
static volatile uint32_t tempo_passado = 0; // Tempo do último evento em microssegundos.
static volatile int numero_na_matriz = 0; // Registro do número que deve aparecer na tela.

const uint8_t quantidade[10] = {12, 8, 11, 10, 9, 11, 12, 8, 13, 12}; // Quantidade de LEDs que serão ativados para cada número.
const uint8_t coordenadas_numero[10][13] = { // Vetor com a identificação dos LEDs que serão ativados para cada número.
    {1, 2, 3, 6, 8, 11, 13, 16, 18, 21, 22, 23}, // 0
    {1, 2, 3, 7, 12, 16, 17, 22}, // 1
    {1, 2, 3, 6, 11, 12, 13, 18, 21, 22, 23}, // 2
    {1, 2, 3, 8, 11, 12, 18, 21, 22, 23}, // 3
    {1, 8, 11, 12, 13, 16, 18, 21, 23}, // 4
    {1, 2, 3, 8, 11, 12, 13, 16, 21, 22, 23}, // 5
    {1, 2, 3, 6, 8, 11, 12, 13, 16, 21, 22, 23}, // 6
    {1, 8, 11, 16, 18, 21, 22, 23}, // 7
    {1, 2, 3, 6, 8, 11, 12, 13, 16, 18, 21, 22, 23}, // 8
    {1, 2, 3, 8, 11, 12, 13, 16, 18, 21, 22, 23} // 9
};

// Protótipos
void inicializacao_maquina_pio(uint pino);
void atribuir_cor_ao_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b);
void limpar_o_buffer(void);
void escrever_no_buffer(void);

static void gpio_irq_handler(uint pino, uint32_t evento);
void inicializacao_dos_pinos(void);
void manipulacao_matriz(void);

// Inicializa a máquina PIO para controle da matriz de LEDs.
void inicializacao_maquina_pio(uint pino){
	uint programa_pio, i;
	// Cria programa PIO.
	programa_pio = pio_add_program(pio0, &ws2818b_program);
	maquina_pio = pio0;

	// Toma posse de uma máquina PIO.
	variavel_maquina_de_estado = pio_claim_unused_sm(maquina_pio, false);
	if (variavel_maquina_de_estado < 0) {
		maquina_pio = pio1;
		variavel_maquina_de_estado = pio_claim_unused_sm(maquina_pio, true); // Se nenhuma máquina estiver livre, panic!
	}

	// Inicia programa na máquina PIO obtida.
	ws2818b_program_init(maquina_pio, variavel_maquina_de_estado, programa_pio, pino, 800000.f);

	// Limpa buffer de pixels.
	for (i = 0; i < CONTADOR_LED; ++i) {
		leds[i].R = 0;
		leds[i].G = 0;
		leds[i].B = 0;
	}
}

// Atribui uma cor RGB a um LED.
void atribuir_cor_ao_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b){
	leds[indice].R = r;
	leds[indice].G = g;
	leds[indice].B = b;
}

// Limpa o buffer de pixels.
void limpar_o_buffer(void){
	for (uint i = 0; i < CONTADOR_LED; ++i)
		atribuir_cor_ao_led(i, 0, 0, 0);
}

// Escreve os dados do buffer nos LEDs.
void escrever_no_buffer(void){
	// Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
	for (uint i = 0; i < CONTADOR_LED; ++i){
		pio_sm_put_blocking(maquina_pio, variavel_maquina_de_estado, leds[i].G);
		pio_sm_put_blocking(maquina_pio, variavel_maquina_de_estado, leds[i].R);
		pio_sm_put_blocking(maquina_pio, variavel_maquina_de_estado, leds[i].B);
	}
}


int main(void){
	inicializacao_maquina_pio(PINO_MATRIZ_LED); // Inicializa matriz de LEDs NeoPixel.
	limpar_o_buffer(); // Limpa o buffer da matriz de LEDs.
	escrever_no_buffer(); // Escreve os dados nos LEDs.

    inicializacao_dos_pinos(); // Inicializa os pinos dos botões e LED vermelho.

    // Usado aqui para ativar a matriz de LEDs com o número inicial zero.
    manipulacao_matriz();

	while(true){
        gpio_put(PINO_LED_VERMELHO, 1);
        sleep_ms(100);
        gpio_put(PINO_LED_VERMELHO, 0);
        sleep_ms(100);
	}
}

static void gpio_irq_handler(uint pino, uint32_t evento){
    // Obtém o tempo atual em microssegundos.
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    // Verificação de passagem de tempo suficiente para tratamento do bouncing.
   
    if(tempo_atual - tempo_passado > 200000){
        tempo_passado = tempo_atual; // Atualização do tempo do último evento.
        if(pino == PINO_BOTAO_A){
            numero_na_matriz++;
            if(numero_na_matriz > 9)
                numero_na_matriz = 0;
        }else if(pino == PINO_BOTAO_B){
            numero_na_matriz--;
            if(numero_na_matriz < 0)
                numero_na_matriz = 9;
        }
        manipulacao_matriz();
    }
}

void inicializacao_dos_pinos(void){
    gpio_init(PINO_BOTAO_A);
    gpio_init(PINO_BOTAO_B);
    gpio_init(PINO_LED_VERMELHO);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
    gpio_set_dir(PINO_LED_VERMELHO, GPIO_OUT);
    gpio_pull_up(PINO_BOTAO_A);
    gpio_pull_up(PINO_BOTAO_B);
    // Configuração da interrupção com callback pelo botão A.
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Configuração da interrupção com callback pelo botão B.
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

void manipulacao_matriz(void){
    limpar_o_buffer();

    for(uint i = 0; i < quantidade[numero_na_matriz]; i++)
        atribuir_cor_ao_led(coordenadas_numero[numero_na_matriz][i], 0, 0, INTENSIDADE_COR_LED);
    
    escrever_no_buffer();
}
