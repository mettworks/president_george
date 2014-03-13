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

int ichbinaus=0;
unsigned int memory[MEM_SIZE];
int mod;
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

unsigned int f=0;

//
// Counter für S-Meter
unsigned int adccounter=20;
unsigned int adcvalues[20];
#define ADCMESSUNGEN 20;

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

ISR (INT7_vect)
{
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
void set_timer3(char status)
{
  if(status == 0)
  {
    #ifdef debug
    uart_puts("Timer3 gestoppt\r\n");
    #endif
    ETIMSK &= ~(1 << OCIE3A);
  }
  else
  {
    #ifdef debug
    uart_puts("Timer3 gestartet\r\n");
    #endif
    ETIMSK |= (1 << OCIE3A);
  }
}
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
    /*
    uart_puts("Durchschnitt: ");
    uart_puts( itoa( sum, s, 10 ) );
    uart_puts("\r\n");
    */
    display_write_meter(sum);
    adccounter=ADCMESSUNGEN;
  }
  else
  {
    adcvalues[adccounter]=messwert;
    adccounter--;
  }
}

void adc_init(void)
{
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescaler to 128 - 125KHz sample rate @ 16MHz
  ADMUX |= (1 << REFS0) | (1 << REFS1); 	// 2.65 V

  ADCSRA |= (1 << ADEN);  // Enable ADC
  ADCSRA |= (1 << ADSC);  // Start A2D Conversions	
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

void toogle_f(void)
{
  set_timer3(0);
  display_del_function();
  f=0;
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

void init_timer0(void)
{
  //
  // http://timogruss.de/2013/06/die-timer-des-atmega128-ctc-modus-clear-timer-on-compare/
  // das ist Timer0
  // Register TIMSK, Bit OCIE0 startet den INT
  //
  // CTC Modus, Vorteiler 128
  TCCR0 |= (1 << WGM01)|(1 << CS02)|(1 << CS00);
  TCNT0 = 0x00; //Timer 0 mit Null initialisieren
  OCR0 = 64;  //Vergleichsregister initialisieren
}
void init_timer3(void)
{
  //
  // das ist Timer3
  //
  TCCR3A |= (1 << WGM31) | (1<<COM3A1);

  TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS30) | (1 << CS32);

  ICR3 = 65535; //TOP
  OCR3A = 65535;

}
//
// vielleicht noch auf 1/10 Hz genau? :-D
void init_tone(void)
{
  // 
  // Anregung: http://www.mikrocontroller.net/topic/215420
  DDRB |= (1<<PB5); 

  TCCR1A |= (1 << WGM11) | (1<<COM1A1);
  // 64 ist Vorteiler
  TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS10) | (1 << CS11);
}
void tone(unsigned int tonefreq)
{
  if(tonefreq == 0)
  {
    ICR1=0;
    OCR1A=0;
  }
  else
  {
    ICR1 = (((18432000/64) / tonefreq) - 1); //TOP
    OCR1A = (((18432000/64) / tonefreq) - 1)/2;
  }
}

void set_timer0(char status)
{
  if(status == 0)
  {
    #ifdef debug
    uart_puts("Timer0 gestoppt\r\n");
    #endif
    TIMSK &= ~(1 << OCIE0);
  }
  else
  {
    #ifdef debug
    uart_puts("Timer0 gestartet\r\n");
    #endif
    TIMSK |= (1 << OCIE0);
  }
}



void set_timer1(char status)
{
  /*
  if(status == 0)
  {
    #ifdef debug
    uart_puts("Timer1 gestoppt\r\n");
    #endif
    TIMSK &= ~(1 << OCIE1A);
  }
  else
  {
    #ifdef debug
    uart_puts("Timer1 gestartet\r\n");
    #endif
    TIMSK |= (1 << OCIE1A);
  }
  */
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
    uart_puts("Wachhund Fehler");
  }
  //wdt_enable(WDTO_2S);         // Watchdog mit 2 Sekunden
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

  //led_helligkeit1(25,0);
  //led_helligkeit2(25,0);
  //display_write_modus(0);
  //adc_init();

  //init_timer0();
  //init_tone();
  //init_timer1();
  //set_timer1(1);

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

  //_delay_ms(1500);
  //wdt_reset();
  //_delay_ms(1500);
  wdt_reset();
  led_helligkeit1(led_br,led_color_v);
  led_helligkeit2(led_br,led_color_v);
  //
  //sei();
  //set_timer3(1);
  while(1)
  {
    //wdt_reset();

    //set_timer3(1);
    //_delay_ms(1000);
    //uart_puts(".");

    //_delay_ms(100);
    //zaehler++;
    //uart_puts("-");
    //messung_s();
  }
} 
