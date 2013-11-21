#include <avr/io.h>
#include <util/delay.h>
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

void data0(void)
{
  PORTA &= ~(1<<PA5);	  // Data 0
	_delay_us(6);
	PORTA |= (1<<PA3);	  // Clock 1
  _delay_us(6);
	PORTA &= ~(1<<PA3);	  // Clock 0
	_delay_us(10);
}
void data1(void)
{
  PORTA |= (1<<PA5);	  // Data 1
  _delay_us(6); 
	_delay_us(30);
  PORTA |= (1<<PA3);	  // Clock 1
  _delay_us(6);
	PORTA &= ~(1<<PA5);	  // Data 0
  PORTA &= ~(1<<PA3);     // Clock 0
	_delay_us(10);
}
//
// Treiber
void begin0(void)
{
  PORTA &= ~(1<<PA3);	// Clock 0
  PORTA &= ~(1<<PA5);   // Data 0
  PORTA &= ~(1<<PA6);   // LE 0
  _delay_us(56);
}
void end0(void)
{
	_delay_us(6);
  PORTA &= ~(1<<PA3);    // Clock 0
  PORTA |= (1<<PA6);	// LE1 1
  _delay_us(6);
  PORTA &= ~(1<<PA6);	//LE1 0
}
//
// PLL
void begin1(void)
{
  PORTA &= ~(1<<PA3);	// Clock 0
  PORTA &= ~(1<<PA5);   // Data 0
  PORTA &= ~(1<<PA4);   // LE 0
  _delay_us(56);
}
void end1(void)
{
	_delay_us(6);
  //PORTA &= ~(1<<PA3);    // Clock 1
  PORTA |= (1<<PA4);			// LE1	1
  _delay_us(6);
  PORTA &= ~(1<<PA4);			// LE1 0
}

void treiber(unsigned int wert)
{
  #ifdef debug 
  uart_puts("Treiber\r\n");
  uart_puts("Bitfolge fuer Treiberbaustein:\t");
  #endif
  begin0();
  int index[16];
  int i;
  for(i=0; wert > 0; i++)
  {
    index[i]=wert%2;
    wert=wert/2;
  }
  for(;i < 16; i++)
  {
    index[i]=0;
  }

  for (; i > 0; i--)
  {
		if(i == 8)
		{
			#ifdef debug
			uart_puts(" ");
			#endif
		}
    if(index[i-1] == 0)
    {
      #ifdef debug
      uart_puts("0");
      #endif
      data0();
    }
    else
    {
      #ifdef debug
      uart_puts("1");
      #endif
      data1();
    }
  }
  end0();
  #ifdef debug
  uart_puts("\r\n");
  #endif
} 

