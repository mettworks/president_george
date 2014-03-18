#include <avr/interrupt.h>
#include <util/delay.h>
#include "i2c.h"
#include "led.h"
#include "transceiver.h"
#include "display.h"
#include <avr/wdt.h>
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

unsigned long keys;
extern int led_farbe;
extern unsigned int led_dimm1;
extern unsigned int led_dimm2;
extern unsigned int step;
extern unsigned int freq;
extern int mod;
extern int cb_mod;
extern int txstat;
extern int modus;
extern unsigned int memory[MEM_SIZE];
extern int cb_channel;
extern unsigned int ctcss;
extern unsigned int rpt;
extern unsigned int echo_ham;
extern unsigned int beep_ham;
extern unsigned int led_br;
extern unsigned int led_color_v;
extern unsigned int f;

extern unsigned long freq_a;
extern unsigned long freq_b;
extern int vfo;
void boot(void)
{
  #ifdef debug
  uart_puts("boot()\r\n");
  #endif
  //
  // Ein und Ausgaeng
  // PE4, INT4 ist VCC Kontrolle                                -> Eingang
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

  // PE4
  DDRE &= ~(1<<PE4);    // Eingang
  //PORTE |= (1<<PE4);  // internen Pullup aktivieren

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
  EICRB |= (0 << ISC40) | (1 << ISC41);    // fallende Flanke
  EICRB |= (0 << ISC50) | (1 << ISC51);    // fallende Flanke
  EIMSK |= (1 << INT4) | (1<< INT7) | (1<< INT5) | (1<< INT6);

  i2c_init();
  display_init();
  //init_led(ADDR_LED00);
  //init_led(ADDR_LED01);
  init_timer0();
  init_timer3();
  init_tone();
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
  memory[1]=data;
  init_geraet();
}

void keycheck(void)
{
  keys=keysauslesen();
	
  #ifdef debug
  char string[20];
  uart_puts("Daten: ");
  sprintf(string,"%lX",keys);
  uart_puts(string);
  uart_puts("\r\n");
  #endif

  _delay_ms(200);
  
  if((keys & 0x20000) == 0)
  {
    #ifdef debug
    uart_puts("RX->TX\r\n");
    #endif
    if(modus == 2)
    {
      #ifdef debug
      uart_puts("kein Sendebetrieb auf dieser Frequenz...\r\n");
      #endif
    }
    else
    {
      tx();
      display_write_modus(1);
      txstat=1;
    }
  }

  else if((keys & 0x10400) == 0)
  {
    #ifdef debug
    uart_puts("M1 + SELECT\r\n");
    uart_puts("RESET!\r\n");
    #endif
    cli();
    wdt_disable();
    format_memory();
    wdt_enable(WDTO_15MS);
  }

  else if((keys & 0x1) == 0)
  {
    #ifdef debug
    uart_puts("Dimmer\r\n");
    #endif
  }
  else if((keys & 0x8) == 0)
  {
    #ifdef debug
    uart_puts("CH19\r\n");
    #endif
  }
  else if((keys & 0x20000000) == 0)
  {
    #ifdef debug
    uart_puts("DC\r\n");
    #endif
  }
  else if((keys & 0x100) == 0)
  {
    #ifdef debug
    uart_puts("Meter\r\n");
    #endif
    if(led_color_v == 0)
    {
      led_color_v=1;
    }
    else
    {
      led_color_v=0;
    }
    led_helligkeit1(led_br,led_color_v);
    led_helligkeit2(led_br,led_color_v);
  }	
  else if((keys & 0x200) == 0)
  {
    #ifdef debug
    uart_puts("LED Helligkeit\r\n");
    #endif
    if(led_br == 0)
    {
      uart_puts("-> 20\r\n");
      led_br=20;
    }
    else if(led_br == 20)
    {
      uart_puts("-> 100\r\n");
      led_br=100;
    }
    else if(led_br == 255)
    {
      uart_puts("-> 0\r\n");
      led_br=0;
    }
    else
    {
      uart_puts("-> 255\r\n");
      led_br=255;
    }
    led_helligkeit1(led_br,led_color_v);
    led_helligkeit2(led_br,led_color_v);
  }	
  else if((keys & 0x400) == 0)
  {
    #ifdef debug
    uart_puts("SELECT\r\n");
    #endif
    tune(28000,5);
  }
  else if((keys & 0x4000000) == 0)
  {
    #ifdef debug
    uart_puts("SCAN\r\n");
    #endif
  }	
  else if((keys & 0x2000000) == 0)
  {
    #ifdef debug
    uart_puts("Echo\r\n");
    #endif
    if(f == 0)
    {
      if(echo_ham == 0)
      {
	set_echo(1);
      }
      else
      {
	set_echo(0);
      }
    }
    else
    {
      if(beep_ham == 0)
      {
	set_beep(1);
      }
      else
      {
	set_beep(0);
      }
      toogle_f();
    }
  }
  else if((keys & 0x20) == 0)
  {
    #ifdef debug
    uart_puts("VFO\r\n");
    #endif
    if(vfo == 0)
    {
      setvfo(1);
    }
    else
    {
      setvfo(0);
    }
  }
  else if((keys & 0x1000000) == 0)
  {
    #ifdef debug
    uart_puts("Beep\r\n");
    #endif
    if(beep_ham == 0)
    {
      set_beep(1);
    }
    else
    {
      set_beep(0);
    }
  }	
  else if((keys & 0x10000) == 0)
  {
    #ifdef debug
    uart_puts("M1\r\n");
    #endif
    set_modulation(2);
  }
  else if((keys & 0x40000) == 0)
  {
    #ifdef debug
    uart_puts("M2\r\n");
    #endif
    set_modulation(1);
  }
  else if((keys & 0x80000) == 0)
  {
    #ifdef debug
    uart_puts("M3\r\n");
    #endif
    set_modulation(4);
  }
  else if((keys & 0x100000) == 0)
  {
    #ifdef debug
    uart_puts("M4\r\n");
    #endif
    set_modulation(3);
  }

  // PA
  else if((keys & 0x8000000) == 0)
  {
    #ifdef debug
    uart_puts("PA\r\n");
    #endif
    if(modus==1)
    {
      setmodus(2);
    }
    else if(modus==2)
    {
      setmodus(1);
    }
  }
    else if((keys & 0x40) == 0)
  {
    f=1;
    uart_puts("F\r\n");
    init_timer3();
    //set_timer3(1);
    display_write_function();
    set_timer3(1);
  }
  // 
  // Drehschalter + ODER CH+
  else if(((keys & 0x2) == 0) || ((keys & 0x80) == 0))
  {
    #ifdef debug
    uart_puts("Drehschalter + ODER Taster +\r\n");
    #endif
    if(modus==1)
    {
      if(vfo==0)
      {
	freq_a=freq_a+1000;
	tune(freq_a,step);
      }
      else
      {
	freq_b=freq_b+1000;
	tune(freq_b,step);
      }
    }
    else
    {
      #ifdef debug
      uart_puts("Modus CB\r\n");
      #endif
      cb_channel++;
      channel(cb_channel);
    }
  }
  // 
  // Drehschalter - ODER CH-
  else if((keys & 0x4) == 0)
  {
    #ifdef debug
    uart_puts("Drehschalter - ODER Taster -\r\n");
    #endif
    if(modus==1)
    {
      if(vfo==0)
      {
	freq_a=freq_a-1000;
	tune(freq_a,step);
      }
      else
      {
	freq_b=freq_b-1000;
	tune(freq_b,step);
      }
    }
    else
    {
      cb_channel--;
      channel(cb_channel);
    }
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
  /*
  if(keys != 0xffffffff)
  {
    set_timer0(1);
  }
  else
  {
    set_timer0(0);
  }
  */
}
