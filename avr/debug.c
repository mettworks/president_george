#ifdef debug
#define BAUD 115800UL

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/setbaud.h> 
#include <avr/io.h>

// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif


int uart_putc(unsigned char c)
{
  while (!(UCSR1A & (1<<UDRIE1)))  
  {
  }                             
  UDR1 = c;                     
  return 0;
}

void uart_puts (char *s)
{
  while (*s)
  {   
    uart_putc(*s);
    s++;
  }
}

int inituart(void)
{
  //
  // Achtung, wir nutzen die 2. UART!
  UBRR1H = UBRR_VAL >> 8;
  UBRR1L = UBRR_VAL & 0xFF;
  UCSR1B |= (1<<TXEN);			  // UART TX einschalten
  UCSR1C =  (1 << UCSZ1) | (1 << UCSZ0);  // Asynchron 8N1 
	return 0;
}

unsigned char inttochar(unsigned int rein)
{
	//int myInt;
	unsigned char raus;
	raus = (unsigned char)rein;
	return raus;
}
#endif