/*
 *
 *  Created on: 15/12/2014
 *      Author: Manolo
 */





#include "UARTstdio.h"

#include <msp430.h>


void UARTinit(char vel){
	 	 P1SEL2|= BIT1 | BIT2;	//P1.1, P1.2 para la UART
	 	 P1SEL|= BIT1 | BIT2;

	 	 UCA0CTL1 |= UCSWRST;			// Reset uart para configurar
	 	 UCA0CTL1 = UCSSEL_2 | UCSWRST;	// Conf. como asincrono
	 	 switch(vel){
	 	 case 1:
	           UCA0BR0 = 9;     // 1MHz/9=111111,1 (3.5% error)
	           break;
	 	 case 8:
	 	     UCA0BR0 =69;       // 8MHz/69 = 115942 ( 0.6% error)
	 	     break;
	 	 case 12:
	          UCA0BR0 =104;     // 12MHz/104 = 115384 (0.16% error)
	             break;
	 	 case 16:
	         UCA0BR0 =139;      // 16MHz/139 = 115108 (<0.1% error)
	              break;
	 	 }

 	     UCA0CTL1 &= ~UCSWRST;			// Quitar Reset
 	     IFG2 &= ~(UCA0RXIFG);			// Borrar flag
	 	 IE2 &=~ UCA0RXIE;				// Deshabilitar las ints.
}

void UARTprintc(char c){
	while (!(IFG2 & UCA0TXIFG)); 	//espera a Tx libre
	UCA0TXBUF = c;
}

void UARTprint(const char * frase){
	while(*frase)UARTprintc(*frase++);
}


void UARTgets( char *BuffRx, int TMAX){
	int indice=0;
	char caracter;
	do
	{
		while(!(IFG2&UCA0RXIFG));
		caracter=UCA0RXBUF;
		UARTprintc(caracter);
		if((caracter!=10)&&(caracter!=13))
		{
		    BuffRx[indice]=caracter;
		    indice++;
		}
	}while((caracter!=13)&&(indice<TMAX));
	if(indice==TMAX)indice--;
	BuffRx[indice]=0;
}


int UARTgetint(void)
{
    char caracter;
    char Err=0;
    unsigned long num=0;
    unsigned int Num;
    do{
        while(!(IFG2&UCA0RXIFG));
        caracter=UCA0RXBUF;
        if(caracter<'0' || caracter>'9' )
        {
            if ((caracter!=10)&&(caracter!=13))
            {
                Err=1;
            }
        }
        else{
            num*=10;
            num+=(caracter-'0');
            if(num>0xffff) {num-=caracter-'0'; num/=10; Err=2;
            }
        }
        UARTprintc(caracter);
    }while((caracter!=13)&&(Err==0));
    if(Err==1) { Num=0xffff;}
    else {Num=(int)num;}
    return(Num);
}



