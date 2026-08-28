/*
 * lcd_uart.h
 *
 *  Created on: 15/12/2014
 *      Author: Manolo
 */



/* Funciones para el manejo de la consola a través de la UART*/

void UARTinit(char vel);                //Inicializa UART. vel=smclk
void UARTprintc(char c);                //Manda caracter c
void UARTprint(const char * frase);     //manda frase completa
void UARTgets( char *BuffRx, int TMAX); //espera cadena hasta CR+LF o limite de tamaño
int  UARTgetint(void);                  //espera cadena y convierte a entero (0xffff si error)



