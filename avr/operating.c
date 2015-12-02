#include <avr/interrupt.h>
#include <util/delay.h>
#include "i2c.h"
#include "led.h"
#include "transceiver.h"
#include "display.h"
#include "memory.h"
#include <avr/wdt.h>
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

#define HAM_FREQ_MIN 28000000UL
#define HAM_FREQ_MAX 29690000UL
#define CB_CH_MIN 1
#define CB_CH_MAX 80

unsigned long keys;
unsigned int set_step=0;
unsigned int quick=0;
extern int led_farbe;
extern unsigned int led_dimm1;
extern unsigned int led_dimm2;
extern unsigned int step;
extern unsigned int step2;

extern unsigned int freq;
extern int mod;
extern int cb_mod;
extern int txstat;
extern int modus;
extern unsigned char memory[MEM_SIZE];
extern int cb_channel;
extern unsigned int ctcss;
extern unsigned int rpt;
extern unsigned int echo_ham;
extern unsigned int echo_cb;
extern unsigned int beep_ham;
extern unsigned int beep_cb;
extern unsigned int led_br;
extern unsigned int led_color_v;
extern unsigned int f;

extern unsigned long freq_a;
extern unsigned long freq_b;
extern int vfo;
extern unsigned int split;
extern unsigned int ham_mod_a;
extern unsigned int ham_mod_b;

unsigned long mkstep2(unsigned int step2)
{
  unsigned long data=0;
  if(step2 == 0)
  {
    data=1000;
  }
  else if(step2 == 1)
  {
    data=5000;
  }
  else if(step2 == 2)
  {
    data=10000;
  }
  else if(step2 == 3)
  {
    data=25000;
  }
  else if(step2 == 4)
  {
    data=100000;
  }
  else if(step2 == 5)
  {
    data=1000000;
  }
  return data;
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
    TCNT0=0;
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

void set_timer3(char status)
{
  if(status == 0)
  {
    // low
    PORTE &= ~(1<<PE0);
    #ifdef debug
    uart_puts("Timer3 gestoppt\r\n");
    #endif
    // Compare Interrupt abschalten
    ETIMSK &= ~(1 << OCIE3A);
    TCCR3A = 0;
    TCCR3B =0; 
    TCNT3 = 0; //Timer mit Null initialisieren
    OCR3A = 0;  //Vergleichsregister initialisieren
  }
  else
  {
    // high
    PORTE |= (1<<PE0);
    #ifdef debug
    uart_puts("Timer3 gestartet\r\n");
    #endif
    TCCR3A = 0;
    TCCR3B |= (1 << WGM32) | (1 << CS30) | (1 << CS32);
    TCNT3 = 0; //Timer mit Null initialisieren
    OCR3A = 0xffff;  //Vergleichsregister initialisieren

    // 
    // Compare Interrupt einschalten
    ETIMSK |= (1 << OCIE3A);
  }
}
void toogle_f(void)
{
  set_timer3(0);
  display_memory_swap();
  if(f==1)
  {
    f=0;
  }
  else if(set_step == 1)
  {
    set_step=0;
  }
}
void boot(void)
{
  #ifdef debug
  uart_puts("boot()\r\n");
  #endif
  //
  // Ein und Ausgaeng
  // PE7, INT7 ist 1. Port Expander                             -> Eingang
  // PE6, INT6 ist 2. Port Expander                             -> Eingang
  // PE5, INT5 ist Taster 1, Ein/Aus                            -> Eingang
  // PA4 ist Latch PLL                                          -> Ausgang              PIN10 vom Mainboard     
  // PA6 ist Latch Treiber                                      -> Ausgang              PIN7 vom Mainboard     
  // PA5 ist Data                                               -> Ausgang              PIN9 vom Mainboard   
  // PA3 ist Clock                                              -> Ausgang              PIN8 vom Mainboard     
  // PC0 ist LED Rot                                            -> Ausgang
  // PC1 ist LED Grün                                           -> Ausgang
  // PA7 ist Ein/Aus                                            -> Ausgang
  // PA2 ist "Busy" (Rauschsperre offen)                        -> Eingang
  // PB5 ist der Ausgang fuer Rogerbeep etc.			-> Ausgang

  // PE7
  DDRE &= ~(1<<PE7);    // Eingang
  PORTE |= (1<<PE7);    // internen Pullup aktivieren
  // PE6
  DDRE &= ~(1<<PE6);    // Eingang
  PORTE |= (1<<PE6);    // internen Pullup aktivieren
  // PE5
  DDRE &= ~(1<<PE5);    // Eingang
  PORTE |= (1<<PE5);    // internen Pullup aktivieren

  // PC0
  DDRC |= (1<<PC0);
  // PC1
  DDRC |= (1<<PC1);
  // PA7
  DDRA |= (1<<PA7);
  PORTA |= (1<<PA7);    // einschalten
  // PA2
  DDRA &= ~(1<<PA2);    // Eingang
  // PA4
  DDRA |= (1<<PA4);     // Bitbanging SPI
  // PA6
  DDRA |= (1<<PA6);     // Bitbanging SPI
  // PA5
  DDRA |= (1<<PA5);     // Bitbanging SPI
  // PA3
  DDRA |= (1<<PA3);     // Bitbanging SPI
  PORTA &= ~(1<<PA3);
  PORTA &= ~(1<<PA4);
  PORTA &= ~(1<<PA5);
  PORTA &= ~(1<<PA6);

  //
  // Interrupts
  // INT4 wird bei fallender Flanke ausgelöst -> VCC weg
  // INT7 wird für den 1. i2c Port Expander genutzt 
  // (warum nicht bei fallender Flanke? Hmmm!)

  EICRB |= (0 << ISC70) | (0 << ISC71);    // 0 löst aus
  EICRB |= (0 << ISC60) | (0 << ISC61);    // 0 löst aus
  EICRB |= (0 << ISC50) | (0 << ISC51);    // fallende Flanke
  EIMSK |= (1<< INT7) | (1<< INT5) | (1<< INT6);
  i2c_init();
  display_init();
  init_timer0();
  //init_timer3();
  //init_tone();
  init_led(ADDR_LED00);
  init_led(ADDR_LED01);
  #ifdef debug
  uart_puts("boot() ENDE\r\n");
  #endif
}

// 2 Byte zurück
unsigned short keysauslesendirekt(unsigned char destaddr)
{
  unsigned char byte0;
  unsigned char byte1;
  unsigned short alles;
  i2c_start_wait(destaddr);
  i2c_write(0x0);
  i2c_rep_start(destaddr + 1);
  byte0=i2c_readAck();
  byte1=i2c_readAck();
  alles = byte1 + (byte0 << 8);
  return alles;
}

unsigned long keysauslesen(void)
{
  // 
  // es werden gleich wieder Interrupts aktiviert, weil:
  // wenn VCC wegfällt, würde auch die i2c Kommunikation wegbrechen, da die Gegenstellen keine Spannung mehr haben
  // so ist sichergestellt, das wir bei einer hängenden i2c Kommunikation auch den INT4 gefasst bekommen
  // ABER ERST WENN I2C FERTIG IST!
  unsigned short blubb1;
  unsigned short blubb2;
  i2c_init();
  blubb1=keysauslesendirekt(0x40);
  blubb2=keysauslesendirekt(0x42);
  // TODO: Wenn hier der i2c Transfer gestoppt wird hängt sich das Teil beim drücken der runter Taste am Mike auf?? 
  //i2c_stop();
  // INT's abschalten
  //cli();
  keys=(uint32_t)blubb2 + ((uint32_t)blubb1 << 16);
  return keys;
}

void setmodus(int data)
{
  #ifdef debug
  uart_puts("setmodus():\r\n");
  #endif
  if(data==0)
  {
    #ifdef debug
    uart_puts("-> HAM\r\n");
    #endif
    memory[13] &= ~(1 << 6);
  }
  else
  {
    #ifdef debug
    uart_puts("-> CB\r\n");
    #endif
    memory[13] |= ( 1 << 6);
  }
  init_geraet();
}

void keycheck(void)
{
  //quick=0;
  //EIMSK &= ~(1 << INT7);
  keys=keysauslesen();
  //_delay_ms(250);
  /*
  if(quick == 0)
  {
    uart_puts("keycheck(): Verzoegerung\r\n");
    _delay_ms(250);
  }
  else
  {
    uart_puts("keycheck(): KEINE Verzoegerung\r\n");
  }
  */
  //unsigned int quick=0;
  /*
  #ifdef debug
  char string[20];
  uart_puts("Daten: ");
  sprintf(string,"%lX",keys);
  uart_puts(string);
  uart_puts("\r\n");
  #endif
  */
  if((keys & 0x20000) == 0)
  {
    #ifdef debug
    uart_puts("RX->TX\r\n");
    #endif
    quick=0;
    /*
    if(modus == 1)
    {
      #ifdef debug
      uart_puts("kein Sendebetrieb auf dieser Frequenz...\r\n");
      #endif
    }
    else
    {
    */
    if(txstat == 0)
    {
      if(modus == 0)
      {
	if(split == 1)
	{
	  setvfo(1);
	}
      }
      tx();
      display_write_modus(1);
      txstat=1;
    }
    /*
    }
    */
  }

  else if((keys & 0x10400) == 0)
  {
    #ifdef debug
    uart_puts("M1 + SELECT\r\n");
    uart_puts("RESET!\r\n");
    #endif
    quick=0;
    cli();
    wdt_disable();
    format_memory();
    wdt_enable(WDTO_15MS);
  }
  /*
  else if((keys & 0x1) == 0)
  {
    #ifdef debug
    uart_puts("Dimmer\r\n");
    #endif
  }
  */
  /*
  else if((keys & 0x8) == 0)
  {
    #ifdef debug
    uart_puts("CH19\r\n");
    #endif
  }
  */
  else if((keys & 0x20000000) == 0)
  {
    #ifdef debug
    uart_puts("DC\r\n");
    #endif
    quick=0;
  }
  else if((keys & 0x100) == 0)
  {
    #ifdef debug
    uart_puts("Meter\r\n");
    #endif
    quick=0;
    if(led_color_v == 0)
    {
      led_color_v=1;
    }
    else
    {
      led_color_v=0;
    }
    set_led_br1(led_br,led_color_v);
    set_led_br2(led_br,led_color_v);
    save_led_color1(led_color_v);
    save_led_color2(led_color_v);
  }	
  else if((keys & 0x200) == 0)
  {
    #ifdef debug
    uart_puts("LED Helligkeit\r\n");
    #endif
    quick=0;
    if(led_br == 0)
    {
      led_br=20;
    }
    else if(led_br == 20)
    {
      led_br=100;
    }
    else if(led_br == 255)
    {
      led_br=0;
    }
    else
    {
      led_br=255;
    }
    set_led_br1(led_br,led_color_v);
    set_led_br2(led_br,led_color_v);
    save_led_br1(led_br);
    save_led_br2(led_br);
  }	
  else if((keys & 0x400) == 0)
  {
    #ifdef debug
    uart_puts("SELECT\r\n");
    #endif
    quick=0;
  }
  else if((keys & 0x4000000) == 0)
  {
    #ifdef debug
    uart_puts("SCAN\r\n");
    #endif
    quick=0;
  }	
  else if((keys & 0x2000000) == 0)
  {
    #ifdef debug
    uart_puts("Echo\r\n");
    #endif
    quick=0;
    if(f == 0)
    {
      if(modus == 0)
      {
	if(echo_ham == 0)
	{
	  set_echo(1);
	  save_echo(1);
	}
	else
	{
	  set_echo(0);
	  save_echo(0);
	}
      }
      else
      {
	if(echo_cb == 0)
	{
	  set_echo(1);
	  save_echo(1);
	}
	else
	{
	  set_echo(0);
	  save_echo(0);
  	}
      }
    }
    else
    {
      if(modus == 0)
      {
	if(beep_ham == 0)
	{
	  set_beep(1);
	  save_beep(1);
	}
	else
	{
	  set_beep(0);
	  save_beep(0);
	}
      }
      else
      {
	if(beep_cb == 0)
	{
	  set_beep(1);
	  save_beep(1);
	}
	else
	{
	  set_beep(0);
	  save_beep(0);
	}
      }
      toogle_f();
    }
  }
  else if((keys & 0x10) == 0)
  {
    #ifdef debug
    uart_puts("Split\r\n");
    #endif
    quick=0;
    if(split == 0)
    {
      split=1;
    }
    else
    {
      split=0;
    }
    display_rpt(split);
  }

  else if((keys & 0x20) == 0)
  {
    #ifdef debug
    uart_puts("VFO\r\n");
    #endif
    quick=0;
    if(modus == 0)
    {
      if(vfo == 0)
      {
	setvfo(1);
      }
      else
      {
	setvfo(0);
      }
    }
  }
  else if((keys & 0x1000000) == 0)
  {
    #ifdef debug
    uart_puts("Step\r\n");
    #endif
    quick=0;
    if(modus == 0)
    {
      if(set_step==0)
      {
	display_write_step(step2);
	set_timer3(1);
	set_step=1;
      }
      else
      {
	set_timer3(0);
	display_memory_swap();
	set_step=0;
      }
    }
  }	
  else if((keys & 0x10000) == 0)
  {
    #ifdef debug
    uart_puts("M1\r\n");
    #endif
    quick=0;
    if(modus==0)
    {
      if(vfo==0)
      {
	tune(freq_a,step,3);
      }
      else
      {
	tune(freq_b,step,3);
      }
    }
    else
    {
      tune2channel(cb_channel,3);
    }
    save_mod(3);
    display_write_mod(3);
  }
  else if((keys & 0x40000) == 0)
  {
    #ifdef debug
    uart_puts("M2\r\n");
    #endif
    quick=0;
    if(modus==0)
    {
      if(vfo==0)
      {
	tune(freq_a,step,2);
      }
      else
      {
	tune(freq_b,step,2);
      }
    }
    else
    {
      tune2channel(cb_channel,2);
    }
    save_mod(2);
    display_write_mod(2);
  }
  else if((keys & 0x80000) == 0)
  {
    #ifdef debug
    uart_puts("M3\r\n");
    #endif
    quick=0;
    if(modus==0)
    {
      if(vfo==0)
      {
	tune(freq_a,step,1);
      }
      else
      {
	tune(freq_b,step,1);
      }
    }
    else
    {
      tune2channel(cb_channel,1);
    }
    save_mod(1);
    display_write_mod(1);
  }
  else if((keys & 0x100000) == 0)
  {
    #ifdef debug
    uart_puts("M4\r\n");
    #endif
    quick=0;
    if(modus==0)
    {
      if(vfo==0)
      {
	tune(freq_a,step,0);
      }
      else
      {
	tune(freq_b,step,0);
      }
    }
    else
    {
      tune2channel(cb_channel,0);
    }
    save_mod(0);
    display_write_mod(0);
  }

  // PA
  else if((keys & 0x8000000) == 0)
  {
    #ifdef debug
    uart_puts("HAM/CB\r\n");
    #endif
    quick=0;
    if(modus==0)
    {
      #ifdef debug
      uart_puts("-> CB\r\n");
      #endif
      setmodus(1);
    }
    else
    {
      #ifdef debug
      uart_puts("-> HAM\r\n");
      #endif
      setmodus(0);
    }
  }
  else if((keys & 0x40) == 0)
  {
    #ifdef debug
    uart_puts("F\r\n");
    #endif
    f=1;
    quick=0;
    //init_timer3();
    display_write_function();
    set_timer3(1);
  }
  // 
  // Drehschalter + ODER CH+
  //  0x2
  else if(((keys & 0x1) == 0) || ((keys & 0x2) == 0))
  {
    if((keys & 0x2) == 0)
    {
      quick=1;
    }
    else
    {
      quick=0;
    }
    #ifdef debug
    uart_puts("Drehschalter + ODER Taster +\r\n");
    #endif
    if(set_step == 1)
    {
      if(step2 == 0)
      {
	step2=1;
      }
      else if(step2 == 1)
      {
	step2=2;
      }
      else if(step2 == 2)
      {
	step2=3;
      }
      else if(step2 == 3)
      {
	step2=4;
      }
      else if(step2 == 4)
      {
	step2=5;
      }
      else if(step2 == 5)
      {
	step2=0;
      }
      memory[15] = (memory[15] & 0xf) | ( mod << 3);
      display_write_frequenz(mkstep2(step2));
      set_timer3(1);
    }
    else
    {
      if(modus==0)
      {
	if(vfo==0)
	{
	  freq_a=freq_a+(mkstep2(step2));
    	  if(freq_a > HAM_FREQ_MAX)
	  {
	    freq_a=(unsigned long)HAM_FREQ_MIN;
	  }

	  tune(freq_a,step,ham_mod_a);
	  save_freq(freq_a,vfo);
	  display_write_frequenz(freq_a);
	}
	else
	{
    	  if(freq_b > HAM_FREQ_MAX)
	  {
	    freq_b=(unsigned long)HAM_FREQ_MIN;
	  }
	  freq_b=freq_b+(mkstep2(step2));
	  tune(freq_b,step,ham_mod_b);
	  save_freq(freq_b,vfo);
          display_write_frequenz(freq_b);
	}
      }
      else
      {
	#ifdef debug
	uart_puts("Modus CB\r\n");
	#endif
	cb_channel++;
	if(cb_channel > CB_CH_MAX)
	{
	  cb_channel=CB_CH_MIN;
	}
	tune2channel(cb_channel,cb_mod);
	save_ch(cb_channel);
	display_write_channel(cb_channel);
	display_write_frequenz(ch2freq(cb_channel));
      }
    }
   save2memory();
  }
  // 
  // Drehschalter - ODER CH-
  else if(((keys & 0x4) == 0) || ((keys & 0x8) == 0 ))
  {
    #ifdef debug
    uart_puts("Drehschalter - ODER Taster -\r\n");
    #endif
    if((keys & 0x4) == 0)
    {
      quick=1;
    }
    else
    {
      quick=0;
    }
    if(set_step == 1)
    {
      if(step2 == 5)
      {
	step2=4;
      }
      else if(step2 == 4)
      {
	step2=3;
      }
      else if(step2 == 3)
      {
	step2=2;
      }
      else if(step2 == 2)
      {
	step2=1;
      }
      else if(step2 == 1)
      {
	step2=0;
      }
      else if(step2 == 0)
      {
	step2=5;
      }
      memory[15] = (memory[15] & 0xf) | ( mod << 3);
      display_write_frequenz(mkstep2(step2));
      set_timer3(1);
    }
    else
    {
      if(modus==0)
      {
	if(vfo==0)
	{
	  freq_a=freq_a-(mkstep2(step2));
	  if(freq_a < HAM_FREQ_MIN)
	  {
	    freq_a=(unsigned long)HAM_FREQ_MAX;
	  }
	  tune(freq_a,step,ham_mod_a);
	  save_freq(freq_a,vfo);
          display_write_frequenz(freq_a);
	}
	else
	{
	  freq_b=freq_b-(mkstep2(step2));
	  if(freq_b < HAM_FREQ_MIN)
	  {
	    freq_b=(unsigned long)HAM_FREQ_MAX;
	  }
	  tune(freq_b,step,ham_mod_b);
	  save_freq(freq_b,vfo);
          display_write_frequenz(freq_b);
	}
      }
      else
      {
	cb_channel--;
	if(cb_channel < CB_CH_MIN)
	{
	  cb_channel=CB_CH_MAX;
	}
	tune2channel(cb_channel,cb_mod);
	save_ch(cb_channel);
	display_write_channel(cb_channel);
	display_write_frequenz(ch2freq(cb_channel));
      }
    }
    save2memory();
  }	
	
  // AM ENDE LASSEN!
  // TX Ende, PTT Taste ist losgelassen
  else if(keys & 0x20000)
  {
    if(txstat==1)
    {
      #ifdef debug
      uart_puts("TX->RX\r\n");
      #endif
      //rogerbeep();
      display_write_modus(0);
      if(modus == 0)
      {
	if(split == 1)
	{
	  setvfo(0);
	}
      }
      rx();
      txstat=0;
    }
  }	
  else
  {
    #ifdef debug
    uart_puts("unbekannter Eingang\r\n");
    #endif
  }
  if(keys != 0xffffffff)
  {
    if(quick==0)
    {
      set_timer0(1);
    }
  }
  else
  {
    if(quick==0)
    {
      set_timer0(0);
    }
  }
  if(quick==1)
  {
    #ifdef debug
    uart_puts("quick -> 1\r\n");
    #endif
    //EIMSK |= (1 << INT7);
    _delay_ms(70);
  }
  else
  {
    #ifdef debug
    uart_puts("quick -> 0\r\n");
    #endif
    //EIMSK |= (1 << INT7);
    _delay_ms(250);
  }
}
