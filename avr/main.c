/*
avrdude -p atmega128 -P /dev/ttyACM0 -c stk500v2 -v -Uefuse:w:0xFF:m -U hfuse:w:0xC9:m -U lfuse:w:0xDF:m

neuer Versuch
Brown Out Detection 2.7V
avrdude -p atmega128 -P /dev/ttyACM0 -c stk500v2 -v -U lfuse:w:0x9f:m -U hfuse:w:0xc9:m -U efuse:w:0xff:m 

*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "memory.h"
#include "display.h"
#include "led.h"
#include "transceiver.h"
#include "operating.h"
#include "i2c.h"
//#include <avr/wdt.h> 
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

int ichbinaus;
unsigned char memory[MEM_SIZE];
unsigned int mod;
unsigned int freq;
unsigned int cb_channel;
unsigned int cb_mod;
unsigned int step = 5;
unsigned int step2=0;
unsigned int ctcss;
unsigned int rpt;
unsigned int echo_ham;
unsigned int echo_cb;
unsigned int beep_ham;
unsigned int beep_cb;
unsigned int beep;

//unsigned int c_timer3=0;

int txstat=0;
int modus;
unsigned int led_br;
unsigned int led_color_v;

unsigned long freq_a;
unsigned long freq_b;
unsigned int vfo;
unsigned int f=0;
unsigned int ham_mod_a;
unsigned int ham_mod_b;
unsigned int split;

ISR (INT5_vect)
{
  #ifdef debug
  uart_puts("INT5_vect()\r\n");
  #endif
  off();
}
ISR (INT7_vect)
{
  //wdt_reset();
  #ifdef debug
  uart_puts("INT7\r\n");
  #endif
  keycheck();
}
ISR (INT6_vect)
{
  #ifdef debug
  uart_puts("INT6\r\n");
  #endif
  while(1)
  {
  }
}
ISR (INT0_vect)
{
  #ifdef debug
  uart_puts("INT0\r\n");
  #endif
  while(1)
  {
  }
}
ISR (INT1_vect)
{
  #ifdef debug
  uart_puts("INT1\r\n");
  #endif
  while(1)
  {
  }
}
ISR (INT2_vect)
{
  #ifdef debug
  uart_puts("INT2\r\n");
  #endif
  while(1)
  {
  }
}
ISR (INT3_vect)
{
  #ifdef debug
  uart_puts("INT3\r\n");
  #endif
  while(1)
  {
  }
}
/*
//
// sehr unelegant, muss mit einem Timer gemacht werden
// erstmal geht es nur um den HW Test ... ^^
void scan(void)
{
  while(1)
  {
    freq=freq+step;
    tune(freq,step);
    _delay_ms(250);
    if ( !(PINA & (1<<PINA2)) )
    {
      #ifdef debug
      uart_puts("Treffer\r\n");
      #endif
      break;
    }
  }
}
*/


ISR (TIMER0_COMP_vect)
{
  #ifdef debug
  uart_puts("INT Timer0\r\n");
  #endif
  keycheck();
}
ISR (TIMER3_COMPA_vect)
{
  #ifdef debug
  uart_puts("INT TIMER3_COMPA_vect\r\n");
  #endif
  toogle_f();
}

//
// IRQ fuer tonausgabe
ISR (TIMER1_COMPA_vect) 
{
  #ifdef debug
  uart_puts("INT Timer1A\r\n");
  #endif
 // PORTD ^= (1<<PD1);
  //PORTB ^= (1 << 5);
}
ISR (TIMER1_COMPB_vect) 
{
  #ifdef debug
  uart_puts("INT Timer1B\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER1_OVF_vect) 
{
  #ifdef debug
  uart_puts("INT Timer1 OVF\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER1_CAPT_vect) 
{
  #ifdef debug
  uart_puts("INT Timer1 CAPT\r\n");
  #endif
  while(1)
  {
  }
}

ISR (TIMER2_COMP_vect)
{
  #ifdef debug
  uart_puts("INT TIMER2_COMP_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER2_OVF_vect)
{
  #ifdef debug
  uart_puts("INT TIMER2_OVF_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER0_OVF_vect)
{
  #ifdef debug
  uart_puts("INT TIMER0_OVF_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (SPI_STC_vect)
{
  #ifdef debug
  uart_puts("INT TIMER0_OVF_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (USART0_RX_vect)
{
  #ifdef debug
  uart_puts("INT USART0_RX_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (USART0_UDRE_vect)
{
  #ifdef debug
  uart_puts("INT USART0_UDRE_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (USART0_TX_vect)
{
  #ifdef debug
  uart_puts("INT USART0_TX_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (ADC_vect)
{
  #ifdef debug
  uart_puts("INT ADC_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (EE_READY_vect)
{
  #ifdef debug
  uart_puts("INT EE_READY_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (ANALOG_COMP_vect)
{
  #ifdef debug
  uart_puts("INT ANALOG_COMP_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER1_COMPC_vect)
{
  #ifdef debug
  uart_puts("INT TIMER1_COMPC_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER3_CAPT_vect)
{
  #ifdef debug
  uart_puts("INT TIMER3_CAPT_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER3_COMPB_vect)
{
  #ifdef debug
  uart_puts("INT TIMER3_COMPB_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER3_COMPC_vect)
{
  #ifdef debug
  uart_puts("INT TIMER3_COMPC_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TIMER3_OVF_vect)
{
  #ifdef debug
  uart_puts("TIMER3_OVF_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (USART1_RX_vect)
{
  #ifdef debug
  uart_puts("INT USART1_RX_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (USART1_UDRE_vect)
{
  #ifdef debug
  uart_puts("INT USART1_UDRE_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (USART1_TX_vect)
{
  #ifdef debug
  uart_puts("INT USART1_TX_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (TWI_vect)
{
  #ifdef debug
  uart_puts("INT TWI_vect\r\n");
  #endif
  while(1)
  {
  }
}
ISR (SPM_READY_vect)
{
  #ifdef debug
  uart_puts("INT SPM_READY_vect\r\n");
  #endif
  while(1)
  {
  }
}
void adc_init(void)
{
  ADMUX |= (1<< REFS0) | (1<<MUX0);
  ADCSRA |=  ( 1 << ADEN) |  (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(void)
{
  ADCSRA |= ( 1 << ADSC);
  while ( ADCSRA & (1<<ADSC) )  
  {
  }
  return ADCW;
}

void messung_s(void)
{
  char s[7];
  uint16_t messwert;
  messwert=adc_read();
  uart_puts("Messwert: ");
  uart_puts( itoa( messwert, s, 10 ) );
  uart_puts("\r\n");
  display_write_meter(messwert);
}

int main(void) 
{
  cli();
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  uart_puts("Beginn main()\r\n");
  #endif
  
  boot();
  read_memory();
  init_geraet();
 
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  uart_puts("Init fertig\r\n");
  #endif

  sei();

  adc_init();

  // :-D 
  //tune(24890000,1,2);

// char *testtemp;
/*
  while(1)
{
  uart_puts("START\r\n");
  i2c_init();
   i2c_start_wait(0x50);
   i2c_write(0x0);
    i2c_start_wait(0x51);
    testtemp=i2c_readNak();
    uart_puts("ENDE");
  _delay_ms(2000);
}
*/
/*
  i2c_stop();
  _delay_ms(2000);

*/
/*  while(1)
{
*/

/*
  uart_puts("START\r\n");
  i2c_init();
   i2c_start(0x50);
   i2c_write(0x0);
    i2c_readNak();
    i2c_start(0x51);
    i2c_readNak();
    uart_puts("ENDE\r\n");
  i2c_stop();
  _delay_ms(2000);
*/
/*
  uart_puts("START\r\n");
  i2c_init();
   i2c_start(0x50);
   i2c_write(0x0);
    i2c_readNak();
    i2c_start(0x51);
    i2c_readNak();
    uart_puts("ENDE\r\n");
  i2c_stop();
  _delay_ms(2000);

  uart_puts("\r\n");
*/
/*
}
*/

  while(1)
  { 
    //messung_s();
  }
} 
