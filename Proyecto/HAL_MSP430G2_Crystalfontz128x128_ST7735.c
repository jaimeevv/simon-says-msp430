/* --COPYRIGHT--,BSD
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//*****************************************************************************
//	LIBRERIA BASADA EN LA SUMINISTRADA POR T.I. para el MSP432P401R.
// HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.c -
//           Hardware abstraction layer for using the Educational Boosterpack's
//           Crystalfontz128x128 LCD with MSP-EXP432P401R LaunchPad
//
//*****************************************************************************


#include <HAL_MSP430G2_Crystalfontz128x128_ST7735.h>
#include "grlib.h"
#include <stdint.h>

#include <msp430g2553.h>

void HAL_LCD_PortInit(void)
{
	/* CONFIGURAR LOS PINES PARA EL RESET, RS Y CS*/
	
	//RESET: PIN P2.7
	P2SEL&=~RES_PIN;
	P2SEL2&=~RES_PIN;
	P2DIR|=RES_PIN;
	// RS: P2.6
	P2SEL&=~RS_PIN;
	P2SEL2&=~RS_PIN;
	P2DIR|=RS_PIN;
		
	// CS: P2.3
	P2SEL&=~CS_PIN;
	P2SEL2&=~CS_PIN;
	P2DIR|=CS_PIN;

}

void HAL_LCD_SpiInit(void)
{
	  LCD_MOSI_PORT_SEL1 |= LCD_MOSI_PIN;
	    LCD_MOSI_PORT_SEL2 |= LCD_MOSI_PIN;

	    // Configure:
	    // LCD_SCLK_PIN for SPI_CLK mode
	    LCD_SCLK_PORT_SEL1 |= LCD_SCLK_PIN;
	    LCD_SCLK_PORT_SEL2 |= LCD_SCLK_PIN;
    
	    // Configure USCIB0 for SPI mode:
	    // SPI master,
	    // 3-pin SPI,
	    // 8-bit data,
	    // Data is captured on the first UCLK edge and changed on the following edge
	    // SMCLK sources USCI clock
	    UCB0CTL1 |= UCSWRST;

	    UCB0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC;
	    UCB0CTL1 |= UCSSEL_3;
	    UCB0BR0 |= 0x00;
	    UCB0BR1 = 0;

	    // Initialize USCI state machine
	    UCB0CTL1 &= ~UCSWRST;
	    CS_LOW;
	    RS_HIGH;

}


//*****************************************************************************
//
// Writes a command to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//
//*****************************************************************************
void HAL_LCD_writeCommand(uint8_t command)
{
	RS_LOW;
	CS_LOW;
	 UCB0TXBUF = command;
	 while(!(IFG2 & UCB0RXIFG));
command=UCB0RXBUF;
	 CS_HIGH;
RS_HIGH;

}


//*****************************************************************************
//
// Writes a data to the CFAF128128B-0145T.  This function implements the basic SPI
// interface to the LCD display.
//
//*****************************************************************************
void HAL_LCD_writeData(uint8_t data)
{

	CS_LOW;
		 UCB0TXBUF = data;
		 while(!(IFG2 & UCB0RXIFG));
data=UCB0RXBUF;
CS_HIGH;
 }



