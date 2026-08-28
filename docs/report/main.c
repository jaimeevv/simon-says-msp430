#include <msp430.h>
#include <stdio.h>
#include <stdint.h>

#include "grlib.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_MSP430G2_Crystalfontz128x128_ST7735.h"

int i=0;

// Notas musicales y sus frecuencias
#define DO  262
#define MI  330
#define SOL 392
#define SI  494
#define DO2 523

// Notas extra para la victoria final
#define RE   294
#define FA   349
#define LA   440
#define RE2 587
#define MI2 659
#define SOL2 784

#define GRAPHICS_COLOR_DARK_YELLOW 0x008B5400
#define GRAPHICS_COLOR_BRIGHT_GREEN   0x007FFF00   // verde muy vivo

Graphics_Context g_sContext;

// Modo accesible: 0 = colores, 1 = simbolos
unsigned char modo_simbolos = 0;

// Flags de despertar
volatile unsigned char tick = 0;
volatile unsigned char start = 0;
volatile unsigned long contador_ticks = 0;

// Boton 1 (P1.1)
volatile unsigned char boton1 = 0;

// Joystick inicial
volatile unsigned int ejex = 512;
volatile unsigned int ejey = 512;

// Funcion reloj
void Set_Clk(char VEL){
    BCSCTL2 = SELM_0 | DIVM_0 | DIVS_0;
    switch(VEL){
    case 16:
        if (CALBC1_16MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_16MHZ;
            DCOCTL = CALDCO_16MHZ;
        }
        break;
    case 8:
        if (CALBC1_8MHZ != 0xFF) {
            __delay_cycles(100000);
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_8MHZ;
            DCOCTL = CALDCO_8MHZ;
        }
        break;
    default:
        if (CALBC1_1MHZ != 0xFF) {
            DCOCTL = 0x00;
            BCSCTL1 = CALBC1_1MHZ;
            DCOCTL = CALDCO_1MHZ;
        }
        break;
    }
    BCSCTL1 |= XT2OFF | DIVA_0;
    BCSCTL3 = XT2S_0 | LFXT1S_2 | XCAP_1;
}

// Funcion que inicializa el ADC
void inicia_ADC(char canales){
    ADC10CTL0 &= ~ENC;
    ADC10CTL0 = ADC10ON | ADC10SHT_3 | SREF_0 | ADC10IE;
    ADC10CTL1 = CONSEQ_0 | ADC10SSEL_0 | ADC10DIV_0 | SHS_0 | INCH_0;
    ADC10AE0 = canales;
    ADC10CTL0 |= ENC;
}

// Funcion para leer el valor del ADC
int lee_ch(char canal){
    ADC10CTL0 &= ~ENC;
    ADC10CTL1 &= (0x0fff);
    ADC10CTL1 |= canal<<12;
    ADC10CTL0 |= ENC;
    ADC10CTL0 |= ADC10SC;
    LPM0;
    return (ADC10MEM);
}

// Rutina de interrupcion ADC
#pragma vector=ADC10_VECTOR
__interrupt void ConvertidorAD(void){
    LPM0_EXIT;
}

// Funcion Timer 25ms
void timer_tick(void){
    TA0CTL = TASSEL_2 | ID_3 | MC_1;  // SMCLK/8 = 2MHz
    TA0CCR0 = 49999;                  // 25ms
    TA0CCTL0 = CCIE;
}

// Rutina de interrupcion Timer
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Interrupcion_T0(void){
    tick = 1;
    contador_ticks++;
    LPM0_EXIT;
}

// Funcion que inicializa el boton de start (boton del joystick)
void boton_start(void){
    P2DIR &= ~BIT5;
    P2REN |= BIT5;
    P2OUT |= BIT5;
    P2IFG &= ~BIT5;
    P2IES |= BIT5;
    P2IE  |= BIT5;
}

// Funcion que inicializa el boton 1 (P1.1)
void boton_1(void){
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;
    P1IFG &= ~BIT1;
    P1IES |= BIT1;
    P1IE  |= BIT1;
}

// Rutina de interrupcion Boton Joystick
#pragma vector=PORT2_VECTOR
__interrupt void Interrupcion_P2(void){
    if(!(P2IN & BIT5)){
        start = 1;
    }
    P2IFG &= ~BIT5;
    LPM0_EXIT;
}

// Rutina de interrupcion Boton 1 (P1.1)
#pragma vector=PORT1_VECTOR
__interrupt void Interrupcion_P1(void){
    if(!(P1IN & BIT1)){
        boton1 = 1;
    }
    P1IFG &= ~BIT1;
    LPM0_EXIT;
}

// Buzzer desactivado por defecto
volatile unsigned char buzzer_activo = 0;

// Funcion que inicializa el buzzer
void init_buzzer(void) {
    P2DIR  |= BIT6;
    P2SEL  &= ~BIT6;
    P2SEL2 &= ~BIT6;
    P2OUT  &= ~BIT6;

    TA1CTL = MC_0;
    TA1CCTL0 = 0;
    TA1CCR0 = 1000;
}

// Funcion que activa el sonido
void suena_hz(unsigned int hz){
    unsigned long cuenta_media = 1000000UL / (unsigned long)hz; // 2MHz/(2*hz)

    if(cuenta_media < 10) cuenta_media = 10;
    if(cuenta_media > 65535) cuenta_media = 65535;

    TA1CTL = TASSEL_2 | ID_3 | MC_1;  // SMCLK/8 => 2MHz, UP
    TA1CCR0 = (unsigned int)(cuenta_media - 1);
    TA1CCTL0 = CCIE;

    buzzer_activo = 1;
}

// Funcion que desactiva el sonido
void apaga_sonido(void){
    buzzer_activo = 0;
    TA1CCTL0 &= ~CCIE;
    TA1CTL = MC_0;
    P2OUT &= ~BIT6;
}

// Rutina de interrupcion buzzer
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Interrupcion_T1(void){
    if(buzzer_activo) P2OUT ^= BIT6;
    else               P2OUT &= ~BIT6;
}

// Funcion que segun el color que elijamos con el joystick, sonara un sonido u otro
void sonido_color(unsigned char color){
    // 1: arriba, 2: derecha, 3: abajo, 4: izquierda
    switch(color){
    case 1: suena_hz(DO);  break;
    case 2: suena_hz(MI);  break;
    case 3: suena_hz(SOL); break;
    case 4: suena_hz(SI);  break;
    default: apaga_sonido();  break;
    }
}

// LFSR (generador de secuencia pseudoaleatoria)
static uint16_t lfsr = 0xACE1u;

// Funcion para almacenar la semilla
void semilla(uint16_t s){
    if(s == 0) {
        s = 0xACE1u; // Evitamos semilla a 0
    }
    lfsr = s; // Guardamos la semilla
}

// Funcion para calcular los bits de la semilla
uint16_t lfsr_siguiente(void){
    // Calculamos un nuevo bit como el XOR de varios taps
    uint16_t bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15); // Desplazamos el bit y lo metemos por la izquierda
    return lfsr; // Devolvemos el nuevo valor
}

unsigned char aleatorio_1_4(void){
    return (unsigned char)((lfsr_siguiente() & 0x03) + 1);
}

// Funcion para dibujar una estrella simple (pequena o grande)
void dibuja_estrella(unsigned char x, unsigned char y, unsigned char grande, uint32_t color){
    Graphics_setForegroundColor(&g_sContext, color);

    if(!grande){
        Graphics_Rectangle p = (Graphics_Rectangle){x, y, x, y};
        Graphics_fillRectangle(&g_sContext, &p);
    }
    else{
        Graphics_Rectangle h = (Graphics_Rectangle){(int)(x-2), (int)y, (int)(x+2), (int)y};
        Graphics_Rectangle v = (Graphics_Rectangle){(int)x, (int)(y-2), (int)x, (int)(y+2)};
        Graphics_fillRectangle(&g_sContext, &h);
        Graphics_fillRectangle(&g_sContext, &v);
    }
}

// Funcion principal
int main(void){
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog-timer

    Set_Clk(16); // Reloj de 16 MHz
    inicia_ADC(BIT0 | BIT3); // Joystick A0 y A3

    // Iniciamos lo correspondiente a la pantalla LCD
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128);
    Graphics_setFont(&g_sContext, &g_sFontCm16b);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_clearDisplay(&g_sContext);

    init_buzzer();  // Inicializamos el buzzer
    timer_tick();   // Inicializamos el timer
    boton_start();  // Inicializamos el boton de start
    boton_1();      // Inicializamos el boton 1

    __bis_SR_register(GIE);

    // Dibujo de los elementos del juego
    Graphics_Rectangle boton[4]; // 4 rectangulos del simon dice

    unsigned int xC = 64; // Centro X
    unsigned int yC = 70; // Centro Y
    unsigned int c  = 26; // Tamano del hueco central
    unsigned int t  = 30; // Grosor del anillo

    // Coordenadas del hueco central
    unsigned int x0 = xC - c/2;
    unsigned int x1 = xC + c/2;
    unsigned int y0 = yC - c/2;
    unsigned int y1 = yC + c/2;

    // Coordenadas del contorno exterior del anillo
    unsigned int xL = xC - (c/2 + t);
    unsigned int xR = xC + (c/2 + t);
    unsigned int yT = yC - (c/2 + t);
    unsigned int yB = yC + (c/2 + t);

    unsigned int gJ = 3; // Espacio de separacion entre rectangulos del anillo

    // Coordenadas de los rectangulos
    boton[0] = (Graphics_Rectangle){(int)(x0 + gJ),(int)yT,(int)(xR),(int)y0}; // Amarillo (arriba)
    boton[1] = (Graphics_Rectangle){(int)x1,(int)(y0 + gJ),(int)xR,(int)yB}; // Azul (derecha)
    boton[2] = (Graphics_Rectangle){(int)(xL),(int)y1,(int)(x1 - gJ),(int)yB}; // Verde (abajo)
    boton[3] = (Graphics_Rectangle){(int)xL, (int)yT, (int)x0, (int)(y1 - gJ)}; // Rojo (izquierda)

    // Marco exterior
    Graphics_Rectangle marco = (Graphics_Rectangle){2, 2, 125, 125};

    // Barra de porcentaje
    Graphics_Rectangle barra = (Graphics_Rectangle){12, 8, 110, 18};
    Graphics_Rectangle caja_pct = (Graphics_Rectangle){112, 8, 120, 18};

    // Animacion victoria
    Graphics_Rectangle barra1 = (Graphics_Rectangle){44,90,52,114};
    Graphics_Rectangle barra2 = (Graphics_Rectangle){58,90,66,114};
    Graphics_Rectangle barra3 = (Graphics_Rectangle){72,90,80,114};
    Graphics_Rectangle marco_eq = (Graphics_Rectangle){36,86,88,118};

    // Array colores (arriba, derecha, abajo, izquierda)
    uint32_t color_alto[4], color_bajo[4];

    // Array de simbolos
    static const int8_t simbolos[4][2] = { {'#',0}, {'@',0}, {'$',0}, {'%',0} };

    // Asignamos los colores al array de colores iluminados
    color_alto[0] = GRAPHICS_COLOR_YELLOW;
    color_alto[1] = GRAPHICS_COLOR_BLUE;
    color_alto[2] = GRAPHICS_COLOR_BRIGHT_GREEN;
    color_alto[3] = GRAPHICS_COLOR_RED;

    // Asignamos los colores al array de colores apagados
    color_bajo[0] = GRAPHICS_COLOR_DARK_YELLOW;
    color_bajo[1] = GRAPHICS_COLOR_DARK_BLUE;
    color_bajo[2] = GRAPHICS_COLOR_DARK_GREEN;
    color_bajo[3] = GRAPHICS_COLOR_DARK_RED;

    // Juego
    unsigned char secuencia[32]; // array que almacenara las 32 rondas de juego

    unsigned int ronda = 1;      // Empezamos en la ronda 1
    unsigned int puntuacion = 0; // La puntuacion inicial es 0

    // Definimos las variables de los tiempos
    unsigned int tiempo_base = 1000; // ms
    unsigned int tiempo = 1000;      // ms
    unsigned char modo_rapido = 0;

    unsigned int T = (unsigned int)(tiempo / 25);
    if(T < 2) T = 2;
    unsigned int T4 = (unsigned int)(T / 4);
    if(T4 < 1) T4 = 1;
    unsigned int T2 = (unsigned int)(2 * T);

    // Defino los estados de la FSM
    typedef enum {
        BIENVENIDA=0, MENSAJE_RONDA, MAQUINA, TURNO_JUGADOR,
        VICTORIA, VICTORIA_FINAL, FIN
    } estados_t;

    estados_t estado = BIENVENIDA;

    // Variables auxiliares de la FSM
    unsigned char sub_maquina = 0;
    unsigned int paso_maquina = 0;
    unsigned int paso_jugador = 0;
    unsigned int tms = 0;
    unsigned char estado_joystick = 0;
    unsigned char seleccion = 0;
    unsigned int tflash = 0;

    // Variables victoria
    unsigned int t_vict = 0;
    unsigned int paso_vict = 0;
    unsigned char anim = 0;

    // Variables victoria final
    unsigned int t_victf = 0;
    unsigned int paso_victf = 0;
    unsigned char animf = 0;

    // Melodia victoria final
    #define N_VICTF 18
    static const unsigned int melodia_victf[N_VICTF] = {
        DO,MI,SOL,DO2,RE2,MI2,RE2,DO2,
        0,SOL,LA,SOL,MI,FA,MI,RE,DO,0
    };
    static const unsigned char duracion_victf[N_VICTF] = {
        4,4,4,6,4,4,4,6,
        3,4,4,4,4,4,4,4,8,8
    };

    // Melodia bienvenida
    #define N_BIENV 16
    static const unsigned int melodia_bienv[N_BIENV] = {
        DO,0,MI,0,SOL,0,DO2,0,
        LA,0,SOL,0,FA,0,MI,0
    };
    static const unsigned char duracion_bienv[N_BIENV] = {
        4,2,4,2,4,2,6,3,
        4,2,4,2,4,2,6,6
    };

    unsigned int t_bienv = 0;
    unsigned int paso_bienv = 0;

    // Estrellas victoria final
    #define N_ESTRELLAS 12
    unsigned char star_x[N_ESTRELLAS];
    unsigned char star_y[N_ESTRELLAS];
    unsigned char star_big[N_ESTRELLAS];
    unsigned char star_on[N_ESTRELLAS];

    // Bucle infinito
    while(1){
        LPM0; // Dormimos al micro

        // Entramos cada 25ms
        if(tick){
            tick = 0;
            tms++;

            if(boton1){
                boton1 = 0;
                apaga_sonido();
                estado = VICTORIA_FINAL;
                tms = 0;
            }

            switch(estado){
            // Pantalla de bienvenida
            case BIENVENIDA:
                if(tms == 1){
                    apaga_sonido(); // Buzzer desactivado

                    tiempo = tiempo_base;
                    modo_rapido = 0;

                    Graphics_clearDisplay(&g_sContext);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawStringCentered(&g_sContext, "SIMON DICE", -1, 64, 50, TRANSPARENT_TEXT);
                    Graphics_drawStringCentered(&g_sContext, "PULSA START", -1, 64, 75, TRANSPARENT_TEXT);

                    t_bienv = 0;
                    paso_bienv = 0;
                }

                t_bienv++;

                if(t_bienv == 1){
                    if(melodia_bienv[paso_bienv] == 0){
                        apaga_sonido();
                    }
                    else{
                        suena_hz(melodia_bienv[paso_bienv]);
                    }
                }

                if(t_bienv >= duracion_bienv[paso_bienv]){
                    t_bienv = 0;
                    paso_bienv++;
                    if(paso_bienv >= N_BIENV){
                        paso_bienv = 0;
                    }
                }

                // Si la flag de start esta a 1, iniciamos el juego
                if(start){
                    start = 0;
                    apaga_sonido();

                    // Modo accesible
                    ejex = (unsigned int)lee_ch(0);
                    if(ejex < 100) {
                        modo_simbolos = 1; // Joystick a la izquierda
                    }
                    else {
                        modo_simbolos = 0; // Centro/derecha colores
                    }

                    // Se decide la semilla aleatoria
                    semilla((uint16_t)(contador_ticks ^ 0xBEEF));
                    for(i=0;i<32;i++){
                        secuencia[i] = aleatorio_1_4();
                    }

                    // Reset del juego
                    ronda = 1;
                    puntuacion = 0;
                    paso_maquina = 0;
                    paso_jugador = 0;
                    sub_maquina = 0;

                    // Duracion de la secuencia de la maquina
                    T = (unsigned int)(tiempo / 25);
                    if(T < 2) T = 2;

                    // Tiempo entre pasos de la secuencia
                    T4 = (unsigned int)(T / 4);
                    if(T4 < 1) T4 = 1;

                    // Duracion de la secuencia del jugador
                    T2 = (unsigned int)(2 * T);

                    estado = MENSAJE_RONDA;
                    tms = 0;
                }
                break;

            // Estado de mensaje entre ronda y ronda
            case MENSAJE_RONDA:
                if(tms == 1){
                    char cad[20];
                    apaga_sonido(); // Buzzer desactivado

                    // Dibujamos el texto de por que ronda vamos
                    Graphics_clearDisplay(&g_sContext);
                    sprintf(cad, "RONDA %d", ronda);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawStringCentered(&g_sContext, (int8_t*)cad, -1, 64, 55, TRANSPARENT_TEXT);
                    Graphics_drawStringCentered(&g_sContext, "MIRA...", -1, 64, 80, TRANSPARENT_TEXT);

                    paso_maquina = 0;
                    sub_maquina = 0;
                }

                // Tiempo de espera la pantalla de ronda
                if(tms >= (800/25)){
                    Graphics_clearDisplay(&g_sContext); // Limpiamos la pantalla

                    // Dibujamos el marco
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawRectangle(&g_sContext, &marco);

                    // Dibujamos la barra de progreso de la ronda
                    Graphics_drawRectangle(&g_sContext, &barra);
                    Graphics_drawRectangle(&g_sContext, &caja_pct);
                    Graphics_drawString(&g_sContext, "%", -1, 122, 7, TRANSPARENT_TEXT);

                    // Dibujamos los 4 rectangulos de color o simbolos
                    for(i=0;i<4;i++){
                        if(!modo_simbolos){
                            Graphics_setForegroundColor(&g_sContext, color_bajo[i]);
                            Graphics_fillRectangle(&g_sContext, &boton[i]);
                        }
                        else{
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillRectangle(&g_sContext, &boton[i]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[i],-1,
                                (boton[i].xMin + boton[i].xMax)/2,
                                (boton[i].yMin + boton[i].yMax)/2,TRANSPARENT_TEXT);
                        }
                    }

                    estado = MAQUINA;
                    tms = 0;
                }
                break;

            case MAQUINA:
                // Si ya se han mostrado todos los pasos, pasamos al turno del jugador
                if(paso_maquina >= ronda){
                    apaga_sonido(); // Buzzer desactivado

                    // Redibujamos estado inactivo
                    for(i=0;i<4;i++){
                        if(!modo_simbolos){
                            Graphics_setForegroundColor(&g_sContext, color_bajo[i]);
                            Graphics_fillRectangle(&g_sContext, &boton[i]);
                        }
                        else{
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillRectangle(&g_sContext, &boton[i]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[i],-1,
                                (boton[i].xMin + boton[i].xMax)/2,
                                (boton[i].yMin + boton[i].yMax)/2,TRANSPARENT_TEXT);
                        }
                    }

                    paso_jugador = 0;
                    estado_joystick = 0;
                    estado = TURNO_JUGADOR;
                    tms = 0;
                    break;
                }

                // Subestado para iluminar color
                if(sub_maquina == 0){
                    if(tms == 1){

                        unsigned char color = secuencia[paso_maquina];
                        unsigned char color_aux = color - 1; // Array de 0 a 3

                        if(!modo_simbolos){
                            Graphics_setForegroundColor(&g_sContext, color_alto[color_aux]);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                        }
                        else{
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[color_aux],-1,
                                (boton[color_aux].xMin + boton[color_aux].xMax)/2,
                                (boton[color_aux].yMin + boton[color_aux].yMax)/2,TRANSPARENT_TEXT);
                        }

                        sonido_color(color);

                        // Barra progreso maquina (relleno rojo)
                        Graphics_Rectangle interior = barra;
                        interior.xMin += 1; interior.yMin += 1;
                        interior.xMax -= 1; interior.yMax -= 1;

                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_fillRectangle(&g_sContext, &interior);

                        unsigned int W = interior.xMax - interior.xMin;
                        unsigned int relleno = (unsigned int)((unsigned long)W * (paso_maquina+1) / ronda);
                        if(relleno > 0){
                            Graphics_Rectangle f = interior;
                            f.xMax = f.xMin + relleno;
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
                            Graphics_fillRectangle(&g_sContext, &f);
                        }
                    }

                    // Tras T segundos
                    if(tms >= T){
                        unsigned char color = secuencia[paso_maquina];
                        unsigned char color_aux = color - 1;

                        if(!modo_simbolos){
                            Graphics_setForegroundColor(&g_sContext, color_bajo[color_aux]);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                        }
                        else {
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[color_aux],-1,
                                (boton[color_aux].xMin + boton[color_aux].xMax)/2,
                                (boton[color_aux].yMin + boton[color_aux].yMax)/2,TRANSPARENT_TEXT);
                        }

                        apaga_sonido(); // Buzzer desactivado
                        sub_maquina = 1; // Pasamos al subestado de pausa entre colores
                        tms = 0;
                    }
                }
                else {
                    if(tms >= T4){
                        paso_maquina++; // Siguiente paso
                        sub_maquina = 0;
                        tms = 0;
                    }
                }
                break;

            case TURNO_JUGADOR: {
                // Si se acaba el tiempo, game over
                if(tms >= T2){
                    estado = FIN;
                    tms = 0;
                    apaga_sonido();
                    break;
                }

                if(tms == 1){
                    // Inicializamos los botones apagados
                    for(i=0; i<4; i++){
                        if(modo_simbolos){
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillRectangle(&g_sContext, &boton[i]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[i],-1,
                                (boton[i].xMin + boton[i].xMax)/2,
                                (boton[i].yMin + boton[i].yMax)/2,TRANSPARENT_TEXT);
                        }
                        else{
                            Graphics_setForegroundColor(&g_sContext, color_bajo[i]);
                            Graphics_fillRectangle(&g_sContext, &boton[i]);
                        }
                    }
                }

                // Leer joystick
                ejex = (unsigned int)lee_ch(0);
                ejey = (unsigned int)lee_ch(3);

                int dx = (int)ejex - 512;
                int dy = (int)ejey - 512;

                // Zona muerta
                if(dx < 100 && dx > -100 && dy < 100 && dy > -100){
                    estado_joystick = 0;
                }

                unsigned char elegido = 0;

                if(estado_joystick == 0){
                    if(!(dx < 100 && dx > -100 && dy < 100 && dy > -100)) {
                        estado_joystick = 1;
                        int adx = (dx<0)?-dx:dx;
                        int ady = (dy<0)?-dy:dy;

                        if(adx > ady) {
                            if(dx > 0) elegido = 2;  // Derecha
                            else       elegido = 4;  // Izquierda
                        }
                        else {
                            if(dy > 0) elegido = 1;  // Arriba
                            else       elegido = 3;  // Abajo
                        }
                    }
                }

                // Si se ha elegido algun color
                if(elegido != 0) {
                    unsigned char esperado = secuencia[paso_jugador];
                    {
                        unsigned char color_aux = elegido - 1;
                        if(!modo_simbolos) {
                            Graphics_setForegroundColor(&g_sContext, color_alto[color_aux]);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                        }
                        else {
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[color_aux],-1,
                                (boton[color_aux].xMin + boton[color_aux].xMax)/2,
                                (boton[color_aux].yMin + boton[color_aux].yMax)/2,TRANSPARENT_TEXT);
                        }

                        sonido_color(elegido);
                        seleccion = elegido;
                        tflash = 0;
                    }

                    // Si el color elegido es el correcto
                    if(elegido == esperado){
                        puntuacion++;
                        paso_jugador++;

                        // Barra progreso jugador (relleno blanco)
                        Graphics_Rectangle interior = barra;
                        interior.xMin += 1; interior.yMin += 1;
                        interior.xMax -= 1; interior.yMax -= 1;

                        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                        Graphics_fillRectangle(&g_sContext, &interior);

                        unsigned int W = interior.xMax - interior.xMin;
                        unsigned int relleno = (unsigned int)((unsigned long)W * (paso_jugador) / ronda);
                        if(relleno > 0){
                            Graphics_Rectangle f = interior;
                            f.xMax = f.xMin + relleno;
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_fillRectangle(&g_sContext, &f);
                        }

                        // Si completa la ronda
                        if(paso_jugador >= ronda){
                            estado = VICTORIA;
                            tms = 0;
                            t_vict = 0;
                            paso_vict = 0;
                            anim = 0;
                            apaga_sonido();
                        }
                        else{
                            tms = 0;
                        }
                    }
                    else {
                        estado = FIN; // ERROR
                        tms = 0;
                    }
                }

                if(seleccion != 0){
                    tflash++;
                    if(tflash >= (T/2)) {
                        unsigned char color_aux = seleccion - 1;
                        if(!modo_simbolos) {
                            Graphics_setForegroundColor(&g_sContext, color_bajo[color_aux]);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                        }
                        else {
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                            Graphics_fillRectangle(&g_sContext, &boton[color_aux]);
                            Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                            Graphics_drawStringCentered(&g_sContext,(int8_t*)simbolos[color_aux],-1,
                                (boton[color_aux].xMin + boton[color_aux].xMax)/2,
                                (boton[color_aux].yMin + boton[color_aux].yMax)/2,TRANSPARENT_TEXT);
                        }

                        apaga_sonido();
                        seleccion = 0;
                    }
                }
                break;
            }

            // Estado de ronda completada
            case VICTORIA:
                if(tms == 1) {
                    Graphics_clearDisplay(&g_sContext);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawStringCentered(&g_sContext, "RONDA", -1, 64, 45, TRANSPARENT_TEXT);
                    Graphics_drawStringCentered(&g_sContext, "SUPERADA", -1, 64, 65, TRANSPARENT_TEXT);

                    Graphics_drawRectangle(&g_sContext, &marco_eq);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_fillRectangle(&g_sContext, &barra1);
                    Graphics_fillRectangle(&g_sContext, &barra2);
                    Graphics_fillRectangle(&g_sContext, &barra3);

                    paso_vict = 0;
                    t_vict = 0;
                    anim = 0;
                }

                t_vict++;

                // Musica simple de victoria
                if(paso_vict == 0) suena_hz(DO);
                if(paso_vict == 1) apaga_sonido();
                if(paso_vict == 2) suena_hz(MI);
                if(paso_vict == 3) apaga_sonido();
                if(paso_vict == 4) suena_hz(SOL);
                if(paso_vict == 5) apaga_sonido();
                if(paso_vict == 6) suena_hz(DO2);
                if(paso_vict == 7) apaga_sonido();
                if(paso_vict == 8) suena_hz(SOL);
                if(paso_vict == 9) apaga_sonido();
                if(paso_vict == 10) suena_hz(DO2);
                if(paso_vict == 11) apaga_sonido();

                if((paso_vict % 2) == 0) {
                    if(t_vict >= 6) { t_vict = 0; paso_vict++; }
                }
                else {
                    if(t_vict >= 2) { t_vict = 0; paso_vict++; }
                }

                // Animacion ecualizador
                if((tms % 2) == 0){
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
                    Graphics_fillRectangle(&g_sContext, &barra1);
                    Graphics_fillRectangle(&g_sContext, &barra2);
                    Graphics_fillRectangle(&g_sContext, &barra3);

                    unsigned char h1=8, h2=18, h3=12;
                    if(anim==0) { h1=8;  h2=18; h3=12; }
                    if(anim==1) { h1=18; h2=10; h3=16; }
                    if(anim==2) { h1=12; h2=16; h3=8;  }
                    if(anim==3) { h1=16; h2=8;  h3=18; }

                    if((paso_vict % 2) == 1){ h1 = 6; h2 = 6; h3 = 6; }

                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_Rectangle b1 = barra1; b1.yMin = (unsigned int)(barra1.yMax - h1);
                    Graphics_Rectangle b2 = barra2; b2.yMin = (unsigned int)(barra2.yMax - h2);
                    Graphics_Rectangle b3 = barra3; b3.yMin = (unsigned int)(barra3.yMax - h3);

                    Graphics_fillRectangle(&g_sContext, &b1);
                    Graphics_fillRectangle(&g_sContext, &b2);
                    Graphics_fillRectangle(&g_sContext, &b3);

                    anim++;
                    if(anim >= 4) anim = 0;
                }

                // Fin animacion
                if(paso_vict >= 12) {
                    apaga_sonido();
                    ronda++;
                    if(ronda > 32) {
                        estado = VICTORIA_FINAL;
                        tms = 0;
                        break;
                    }
                    paso_maquina = 0;
                    sub_maquina = 0;
                    paso_jugador = 0;
                    seleccion = 0;
                    estado = MENSAJE_RONDA;
                    tms = 0;
                }
                break;

            // Estado de victoria final
            case VICTORIA_FINAL:
                if(tms == 1){
                    apaga_sonido();
                    Graphics_clearDisplay(&g_sContext);

                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawStringCentered(&g_sContext, "VICTORIA!", -1, 64, 45, TRANSPARENT_TEXT);
                    Graphics_drawStringCentered(&g_sContext, "PULSA START", -1, 64, 95, TRANSPARENT_TEXT);

                    t_victf = 0;
                    paso_victf = 0;
                    animf = 0;

                    for(i=0;i<N_ESTRELLAS;i++){
                        star_x[i] = (unsigned char)(4 + (lfsr_siguiente() % 120));
                        if((lfsr_siguiente() & 1) == 0) star_y[i] = (unsigned char)(8 + (lfsr_siguiente() % 22));
                        else                            star_y[i] = (unsigned char)(85 + (lfsr_siguiente() % 35));
                        star_big[i] = (unsigned char)(lfsr_siguiente() & 1);
                        star_on[i] = 0;
                    }
                }

                t_victf++;

                if(t_victf == 1){
                    if(melodia_victf[paso_victf] == 0) apaga_sonido();
                    else suena_hz(melodia_victf[paso_victf]);
                }

                if(t_victf >= duracion_victf[paso_victf]){
                    t_victf = 0;
                    paso_victf++;
                    if(paso_victf >= N_VICTF) paso_victf = 0;
                }

                if((tms % 2) == 0){
                    unsigned char k = (unsigned char)(lfsr_siguiente() % N_ESTRELLAS);
                    if(star_on[k]){
                        dibuja_estrella(star_x[k], star_y[k], star_big[k], GRAPHICS_COLOR_BLACK);
                        star_on[k] = 0;
                    }else{
                        star_big[k] = (unsigned char)(star_big[k] ^ 1);
                        dibuja_estrella(star_x[k], star_y[k], star_big[k], GRAPHICS_COLOR_YELLOW);
                        star_on[k] = 1;
                    }
                    animf++;
                    if(animf >= 4) animf = 0;
                }

                if(start){
                    start = 0;
                    modo_rapido = 1; // Aumentamos velocidad
                    tiempo = (unsigned int)(tiempo_base/2);

                    T = (unsigned int)(tiempo / 25);
                    if(T < 2) T = 2;
                    T4 = (unsigned int)(T / 4);
                    if(T4 < 1) T4 = 1;
                    T2 = (unsigned int)(2 * T);

                    semilla((uint16_t)(contador_ticks ^ 0xBEEF));
                    for(i=0;i<32;i++) secuencia[i] = aleatorio_1_4();

                    ronda = 1; puntuacion = 0;
                    paso_maquina = 0; paso_jugador = 0;
                    sub_maquina = 0; seleccion = 0;

                    estado = MENSAJE_RONDA;
                    tms = 0;
                    apaga_sonido();
                }
                break;

            // Estado de game over
            case FIN:
                if(tms == 1) {
                    char cad[24];
                    apaga_sonido();

                    if(modo_rapido){
                        modo_rapido = 0;
                        tiempo = tiempo_base;
                        T = (unsigned int)(tiempo / 25); if(T < 2) T = 2;
                        T4 = (unsigned int)(T / 4); if(T4 < 1) T4 = 1;
                        T2 = (unsigned int)(2 * T);
                    }

                    Graphics_clearDisplay(&g_sContext);
                    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
                    Graphics_drawStringCentered(&g_sContext, "GAME OVER", -1, 64, 45, TRANSPARENT_TEXT);
                    sprintf(cad, "PUNTOS: %d", puntuacion);
                    Graphics_drawStringCentered(&g_sContext, (int8_t*)cad, -1, 64, 70, TRANSPARENT_TEXT);
                    Graphics_drawStringCentered(&g_sContext, "PULSA START", -1, 64, 95, TRANSPARENT_TEXT);

                    suena_hz(150); // Sonido derrota
                }
                if(tms >= (600/25)) apaga_sonido();

                if(start) {
                    start = 0;
                    estado = BIENVENIDA;
                    tms = 0;
                }
                break;
            }
        }
    }
}
