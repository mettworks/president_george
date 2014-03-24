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

//int ichbinaus=0;
unsigned int memory[MEM_SIZE];
unsigned int mod;
unsigned int freq;
unsigned int cb_channel;
unsigned int cb_mod;
unsigned int step = 5;
unsigned int ctcss;
unsigned int rpt;
unsigned int echo_ham;
unsigned int beep_ham;
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
  cli();
  wdt_disable();
  #ifdef debug
  uart_puts("INT4_vect()\r\n");
  #endif
  off();
}
/*
ISR (INT5_vect)
{
  #ifdef debug
  uart_puts("INT5_vect()\r\n");
  #endif
  _delay_ms(100);
  //
  // Entprellung
  if ( !(PINE & (1<<PINE5)) ) 
  {
    if(ichbinaus == 1)
    {
      #ifdef debug
      uart_puts("AN\r\n");
      #endif
      PORTA |= (1<<PA7);	// einschalten
      //led_helligkeit1(led_dimm1);
      //led_helligkeit2(led_dimm2);
      init_geraet();
      ichbinaus=0;
      EIMSK |= (1 << INT4) | (1<< INT7) | (1<< INT5) | (1<< INT6);
    }
    else
    {
      ichbinaus=1;
      #ifdef debug
      uart_puts("AUS\r\n");
      #endif
      display_clear();
      save2memory();
      PORTA &= ~(1<<PA7);	// ausschalten...
      //led_helligkeit1(0,0);
      //led_helligkeit2(0,0);
      // LED 9 ist die am Taster 1...
      //led_pwm(10,255,0);
      EIMSK = (1<< INT5);
    }
  }
}
*/
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
}

ISR (TIMER1_COMPA_vect) 
{
  #ifdef debug
  uart_puts("INT Timer1A\r\n");
  #endif
  PORTB ^= (1 << 5);
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
  cli();
  _delay_ms(500);
  if ((MCUCSR & (1 << EXTRF)) || (MCUCSR & (1 << PORF)) || (MCUCSR & (1 << BORF)))              // external, power-on- oder brown-out-reset
  {
    MCUCSR = 0;                                         // Reset-Flags zurücksetzen
  }
  if ((MCUCSR & (1 << WDRF)))           // watchdog-reset
  {
    MCUCSR = 0;                                         // Reset-Flags zurücksetzen
    #ifdef debug
    uart_puts("Wachhund Fehler");
    #endif
  }
  wdt_enable(WDTO_2S);         // Watchdog mit 2 Sekunden
  wdt_reset();
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  uart_puts("Beginn main()\r\n");
  #endif
  
  boot();
  read_memory();
  init_geraet();
 
  _delay_ms(500);

  wdt_reset();

  //adc_init();

  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  uart_puts("Init fertig\r\n");
  #endif

  wdt_reset();
  _delay_ms(500);
  wdt_reset();

  init_led(ADDR_LED00);
  init_led(ADDR_LED01);

  sei();

  led_helligkeit1(0x255,led_color_v);
  led_helligkeit2(0x255,led_color_v);
  
  while(1)
  {
    wdt_reset();
    //messung_s();
  }
} 
