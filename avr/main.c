/*
avrdude -p atmega128 -P /dev/ttyACM0 -c stk500v2 -v -Uefuse:w:0xFF:m -U hfuse:w:0xC9:m -U lfuse:w:0xDF:m
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
#include <avr/wdt.h> 
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
unsigned int led_br=255;
unsigned int led_color_v=0;

unsigned long freq_a;
unsigned long freq_b;
unsigned int vfo;
unsigned int f=0;
unsigned int ham_mod_a;
unsigned int ham_mod_b;
unsigned int split;

//
// Counter für S-Meter
/*
unsigned int adccounter=20;
unsigned int adcvalues[20];
#define ADCMESSUNGEN 20;
*/
//
// IRQ für Spannungsabfall
// wegspeichern der Einstellungen im EEPROM
// beim wiederkommen von VCC, wird durch ein RC Glied Reset ausgelöst
ISR (INT4_vect)
{
  TIMSK=0;
  EIMSK=0;

  cli();
  wdt_disable();
  #ifdef debug
  uart_puts("INT4_vect()\r\n");
  #endif
  off2();
}
ISR (INT5_vect)
{
  #ifdef debug
  uart_puts("INT5_vect()\r\n");
  #endif
  off();
}
ISR (INT7_vect)
{
  wdt_reset();
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
uint16_t adc_read(void )
{
  // ADC1 auswählen
  //ADMUX |= (1<<MUX0);
  //ADMUX |= (1 << MUX3) | (1<<MUX0);
  ADMUX |= (1<<MUX0);
  ADCSRA |= (1<<ADSC);
  while (ADCSRA & (1<<ADSC) ) 
  {   
  }
  return ADCW;                   
}
/*
void messung_s(void)
{
  uint16_t messwert;
  messwert=adc_read();
  uint32_t sum=0;
  unsigned int i;
  //char s[7];
	
  if(adccounter == 0)
  {
    i=ADCMESSUNGEN;
    while(i > 0)
    {
      sum=sum+adcvalues[i];
      i--;
    }
    sum=sum/ADCMESSUNGEN;
    //uart_puts("Durchschnitt: ");
    //uart_puts( itoa( sum, s, 10 ) );
    //uart_puts("\r\n");
    display_write_meter(sum);
    adccounter=ADCMESSUNGEN;
  }
  else
  {
    adcvalues[adccounter]=messwert;
    adccounter--;
  }
}
*/
void adc_init(void)
{
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);	  // Set ADC prescaler to 128 - 125KHz sample rate @ 16MHz
  ADMUX |= (1 << REFS0) | (1 << REFS1);			  // 2.65 V
  ADCSRA |= (1 << ADEN);				  // Enable ADC
  ADCSRA |= (1 << ADSC);				  // Start A2D Conversions	
}

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
  //set_timer3(0);
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

int main(void) 
{
  //
  // Timermessung per Logikanalyzer
  DDRE = (1<<DDE0);
  // low
  PORTE &= ~(1<<PE0);
  cli();
  /*
  if ((MCUCSR & (1 << EXTRF)) || (MCUCSR & (1 << PORF)) || (MCUCSR & (1 << BORF)))    // external, power-on- oder brown-out-reset
  {
    MCUCSR = 0;									      // Reset-Flags zurücksetzen
  }
  if ((MCUCSR & (1 << WDRF)))							      // watchdog-reset
  {
    MCUCSR = 0;									      // Reset-Flags zurücksetzen
    #ifdef debug
    uart_puts("Wachhund Fehler");
    #endif
  }
  wdt_enable(WDTO_2S);								      // Watchdog mit 2 Sekunden
  wdt_reset();
  */
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  uart_puts("Beginn main()\r\n");
  #endif
  
  boot();
  read_memory();
  init_geraet();
 
  //wdt_reset();

  //adc_init();

  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  uart_puts("Init fertig\r\n");
  #endif

  init_led(ADDR_LED00);
  init_led(ADDR_LED01);

  sei();

  led_helligkeit1(0x255,led_color_v);
  led_helligkeit2(0x255,led_color_v);

  while(1)
  { 
    //wdt_reset();
    //messung_s();
    /*
    set_timer3(1);
    _delay_ms(8000);
    set_timer3(1);
    _delay_ms(8000);
    */
  }
} 
