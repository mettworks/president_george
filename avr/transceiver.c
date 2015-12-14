#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "transceiver.h"
#include "3wire.h"
#include "led.h"
#include "display.h"
#include "memory.h"
#include "i2c.h"
#include "operating.h"
//#include "channels.h"
#include <avr/wdt.h>
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif
#include "channels.h"

//
// Bits fuer den Treiberbaustein
// IC406 beginnend bei PIN 16 bis 9
#define TREIBER_MOD 0  
#define TREIBER_FM 1
#define TREIBER_AM 2
#define TREIBER_LSB 3
#define TREIBER_USB 4
#define TREIBER_NBANL 5
#define TREIBER_HICUT 6
#define TREIBER_ECHO 7

// IC405 beginnend bei PIN16 bis 9
#define TREIBER_DIM 8
#define TREIBER_BIT9 9
#define TREIBER_TR 10
#define TREIBER_PA 11
#define TREIBER_MUTE 12
#define TREIBER_CAL 13
#define TREIBER_SWR 14
#define TREIBER_SRF 15

extern unsigned int led_color_v;
extern unsigned int led_br;
extern int ichbinaus;
unsigned int wert = 0;
int ichsende=0;
extern unsigned int freq;
extern unsigned int step;
extern unsigned int cb_channel;
extern unsigned int cb_mod;
extern unsigned int mod;
extern int modus;
extern unsigned int ctcss;
extern unsigned int rpt;
extern unsigned int echo_ham;
extern unsigned int echo_cb;
extern unsigned int beep_ham;
extern unsigned int beep_cb;
extern unsigned int beep;

char string[10];

extern unsigned char memory[MEM_SIZE];
extern unsigned long freq_a;
extern unsigned long freq_b;
extern unsigned int vfo;
extern unsigned int ham_mod_a;
extern unsigned int ham_mod_b;
extern unsigned int step2;

void off(void)
{
  cli();
  _delay_ms(100);
  //
  // Entprellung
  if ( !(PINE & (1<<PINE5)) )
  {
    #ifdef debug
    uart_puts("off():\r\n");
    #endif
    if(ichbinaus == 1)
    {
      #ifdef debug
      uart_puts("AN\r\n");
      #endif
      memory[13] &= ~( 1 << 7);
      PORTA |= (1<<PA7);        // einschalten
      init_geraet();
      //led_helligkeit1(0x255,led_color_v);
      //led_helligkeit2(0x255,led_color_v);
      ichbinaus=0;
      EIMSK |= (1<< INT7) | (1<< INT5) | (1<< INT6);
      sei();
    }
    else
    {
      off2();
    }
  }

}
void off2(void)
{
      ichbinaus=1;
      #ifdef debug
      uart_puts("AUS\r\n");
      #endif
      memory[13] |= ( 1 << 7);
      display_clear();
      save2memory();
      PORTA &= ~(1<<PA7);       // ausschalten...
      set_led_br1(0x0,led_color_v);
      set_led_br2(0x0,led_color_v);
      if(led_color_v == 0)
      {
	led_pwm(7,0x255,ADDR_LED00);
      }
      else
      {
	led_pwm(8,0x255,ADDR_LED00);
      }
      EIMSK = (1<< INT5);
      sei();
      while(1)
      {
      }	
}

int init_geraet(void)
{
  #ifdef debug
  uart_puts("init_geraet()\r\n");
  #endif
  display_clear();
  // alle Bits sind in der gleichen Reihenfolge wie im Schaltplan angegeben
  //
  // Init:
  // 0100 0001  0100 0000
  // Pause, 28ms
  // 0100 1001  0100 0000
  // Pause, 28ms
  wert |= (1 << TREIBER_BIT9);
  wert |= (1 << TREIBER_SRF);
  wert |= (1 << TREIBER_FM);

/*
Aufbau Array "memory":

Byte  0-3 Frequenz in Hz VFO A / HAM  -> han_freq_a 
Byte  4-7 Frequenz in Hz VFO B / HAM  -> ham_freq_b
Byte    8 Channel / CB                -> cb_channel
Byte    9 Brightness / Display
Byte   10 Brightness / Keys
Byte   11 Brightness Standby


Byte 12
#######
    7-6       5-4         3-2     1                     0
1 Mod HAM A   Mod HAM B   Mod CB  color display red	color keys red
0                                 color display green   color keys green
Byte 13
#######
     7    6     5           4               3               2               1             0
1    off  ham   split on    NBANL ham on    NBANL cb on     HICUT ham on    HICUT cb on   VFO B active
0    on   cb	split off   NBANL ham off   NBANL cb off    HICUT ham off   HICUT cb off  VFO A active

Byte 14
#######

  7-6           5-4         3             2           1               0
1 RB Typ HAM    RB Typ CB   RB on HAM     RB on CB    Echo on HAM     Echo on CB
0                           RB off HAM    RB off CB   Echo off HAM    Echo off CB

Byte 15
#######
7-4         3-0
Step VFO A  Step VFO B

Byte 16
#######
  7-1         0
1 CTCSS Tone  CTCSS on
0             CTCSS off

Byte 17
#######
7             6   5-0
1 relais on   -   *100kHz
0 relais off  +

*/
  if(memory[13] &(1<<7))
  {
    uart_puts("ich bleibe aus...\r\n");
    EIMSK = (1<< INT5);
    ichbinaus=1;
    off2();
  }
  else
  {
    ichbinaus=0;
  }


  led_color_v=memory[12] & (1 << 0);
  led_br=memory[10];
  set_led_br1(led_br,led_color_v);
  set_led_br2(led_br,led_color_v);

  if(memory[13] &(1<<6))
  {
    #ifdef debug 
    uart_puts("Modus CB\r\n");
    #endif
    modus=1;
    cb_channel=memory[8];
    cb_mod=memory[12] >> 2 & 0x3;

    tune2channel(cb_channel,cb_mod);
    // TODO!
    //set_modulation(cb_mod);
    echo_cb=memory[14] & 0x01;
    set_echo(echo_cb);
    beep_cb=memory[14] >> 2 & 0x01;
    set_beep(beep_cb);
    display_write_channel(cb_channel);
    display_write_frequenz(ch2freq(cb_channel));
    display_write_mod(cb_mod);
  }
  else
  {
    #ifdef debug 
    uart_puts("Modus HAM\r\n");
    #endif
    modus=0;
    freq_a = ((unsigned long int) memory[0]) + ((unsigned long int) memory[1] << 8) + ((unsigned long int) memory[2] << 16) + ((unsigned long int) memory[3] << 24);
    freq_b = ((unsigned long int) memory[4]) + ((unsigned long int) memory[5] << 8) + ((unsigned long int) memory[6] << 16) + ((unsigned long int) memory[7] << 24);
    step2 = memory[15] >> 3;
    

    if(memory[13] &0) 
    {
      vfo=1;
    }
    else
    {
      vfo=0;
    }
    ham_mod_a = memory[12] >> 6; 
    ham_mod_b = memory[12] >> 4 & 0x3;
    if((26565000 > freq_a) || (29690000 < freq_a) || (freq_a == 0))
    {
      #ifdef debug
      uart_puts("Frequenz A aus dem EEPROM ist falsch!\r\n");
      #endif
      freq_a=28500000;
    }
    if((26565000 > freq_b) || (29690000 < freq_b) || (freq_b == 0))
    {
      #ifdef debug
      uart_puts("Frequenz B aus dem EEPROM ist falsch!\r\n");
      #endif
      freq_b=28500000;
    }
    //tune(freq_a,step);
    _delay_ms(28);
    if(vfo == 0)
    {
      //set_modulation(ham_mod_a);
      tune(freq_a,step,ham_mod_a);
      display_write_mod(ham_mod_a);
      display_write_frequenz(freq_a);
    }
    else
    {
      //set_modulation(ham_mod_b);
      tune(freq_b,step,ham_mod_b);
      display_write_mod(ham_mod_b);
      display_write_frequenz(freq_b);
    }
    setvfo(vfo);
    echo_ham=memory[14] >> 1 & 0x01;
    set_echo(echo_ham);
    beep_ham=memory[14] >> 3 & 0x01;
    set_beep(beep_ham);
  }

  step=5;
  //ctcss=memory[16];
  //rpt=memory[18];
  //echo_ham=memory[12];
  //echo_ham=0;
  //beep_ham=memory[14];
  //ctcss_tone=memory[17];

  treiber(wert); 
  _delay_ms(28);
  wert |= (1 << TREIBER_MUTE);
  treiber(wert);
  _delay_ms(28);
  //set_ctcss(ctcss);
  //set_rpt(rpt);
  //set_echo(echo_ham);
  //set_beep(beep_ham);
  //display_write_vfo('A');
  display_write_modus(0);

  return 0;
}

void set_echo(unsigned int echo_value)
{
  if(echo_value==1)
  {
    #ifdef debug
    uart_puts("set_echo(): AN\r\n");
    #endif
    wert |= (1 << TREIBER_ECHO);
    treiber(wert);
    display_echo(1);
    if(modus==0)
    {
      echo_ham=1;
    }
    else
    {
      echo_cb=1;
    }
  }
  else
  {
    #ifdef debug
    uart_puts("set_echo(): AUS\r\n");
    #endif
    wert &= ~(1 << TREIBER_ECHO);
    treiber(wert);
    display_echo(0);
    if(modus==0)
    {
      echo_ham=0;
    }
    else
    {
      echo_cb=0;
    }
  }
}

void set_beep(unsigned int beep_value)
{
  if(beep_value==1)
  {
    #ifdef debug
    uart_puts("set_beep(): AN\r\n");
    #endif
    display_beep(1);
    if(modus==0)
    {
      beep_ham=1;
    }
    else
    {
      beep_cb=1;
    }

    beep=1;
  }
  else
  {
    #ifdef debug
    uart_puts("set_beep(): AUS\r\n");
    #endif
    display_beep(0);
    if(modus==0)
    {
      beep_ham=0;
    }
    else
    {
      beep_cb=0;
    }
    beep=0;
  }
}

void setvfo(unsigned int vfo_value)
{
  if(vfo_value == 0)
  {
    vfo=0;
    #ifdef debug
    uart_puts("VFO -> A\r\n");
    #endif
    tune(freq_a,step,ham_mod_a);
    display_write_vfo('A');
    memory[13] &= ~(0 << 0);
    display_write_frequenz(freq_a);
    display_write_mod(ham_mod_a);
  }
  else
  {
    vfo=1;
    #ifdef debug
    uart_puts("VFO -> B\r\n");
    #endif
    tune(freq_b,step,ham_mod_b);
    display_write_vfo('B');
    memory[13] |= ( 0 << 0);
    display_write_frequenz(freq_b);
    display_write_mod(ham_mod_b);
  }
}

void set_ctcss(unsigned int ctcss_value)
{
  if(ctcss_value==1)
  {
    #ifdef debug
    uart_puts("set_ctcss(): AN\r\n");
    #endif
    display_ctcss(1);
    ctcss=1;
    memory[16]=1;
  }
  else
  {
    #ifdef debug
    uart_puts("set_ctcss(): AUS\r\n");
    #endif
    display_ctcss(0);
    ctcss=0;
    memory[16]=0;
  }
}

void set_rpt(unsigned int rpt_value)
{
  if(rpt_value==1)
  {
    #ifdef debug
    uart_puts("set_rpt(): AN\r\n");
    #endif
    display_rpt(1);
    rpt=1;
    //memory[18]=1;
  }
  else
  {
    #ifdef debug
    uart_puts("set_rpt(): AUS\r\n");
    #endif
    display_rpt(0);
    rpt=0;
    //memory[18]=0;
  }
}

int tx(void)
{
  if(ichsende != 1)
  {
    ichsende=1;
    /*
    if(rpt==1)
    {
      tune(freq-100,5);
    }
    */
    // alle Bits sind in der gleichen Reihenfolge wie im Schaltplan angegeben
    //
    // Senden:
    // 0100 0001  0100 0000
    // Pause, 7ms
    // 0110 0001  0100 0000
    wert &= ~(1 << TREIBER_MUTE);
    // Hi Power Mode!
    wert &= ~(1 << TREIBER_BIT9);
    treiber(wert);
    _delay_ms(7);
    wert |= (1 << TREIBER_TR);
    treiber(wert);
    _delay_ms(250);
    /*
    if(ctcss==1)
    {
      tone(67);
    }
    */
    //tone(750);
  }
  return 0;
}

int rx(void)
{
  /*
  if(rpt==1)
  {
    tune(freq+100,5);
  }
  */
  /*
  if((beep_ham == 1) || (beep_cb == 1))
  {
    rogerbeep();
  }
  */
  if(beep == 1)
  {
    rogerbeep();
    _delay_ms(100);
  }
  ichsende=0;
  // alle Bits sind in der gleichen Reihenfolge wie im Schaltplan angegeben
  //
  // Empfangen:
  // 0100 0001  0100 0000
  // Pause, 4ms
  // 0100 1001  0100 0000
  // Pause, 4ms
  //rogerbeep();
  wert &= ~(1 << TREIBER_TR);
  treiber(wert);
  _delay_ms(4);
  wert |= (1 << TREIBER_MUTE);
  treiber(wert);
  _delay_ms(4);
  return 0;
}
 
unsigned long ch2freq(unsigned int ch)
{
  #ifdef debug 
  uart_puts("ch2freq(): Kanal ");
  char text[10];
  utoa(ch,text,10);
  uart_puts(text);
  uart_puts(" -> ");
  #endif
  unsigned long data;
  data=(unsigned long)channels[ch-1];
  data=data*1000;
  #ifdef debug
  ltoa(data,text,10);
  uart_puts(text);
  uart_puts("\r\n");
  #endif
  return data;
}

void tune2channel(unsigned int ch,unsigned int cb_mod)
{
  unsigned long cb_freq;
  #ifdef debug 
  uart_puts("channel(): Kanal ");
  char text[10];
  utoa(ch,text,10);
  uart_puts(text);
  uart_puts("\r\n");
  #endif
  cb_freq=ch2freq(ch);
  tune(cb_freq,5,cb_mod);
  cb_channel=ch;
}

int tune(unsigned long freq2tune,unsigned int step2tune,unsigned int mod)
{
  #ifdef debug 
  uart_puts("Modulation: ");
  #endif
  if(mod == 2)
  {
    // 1 FM
    #ifdef debug 
    uart_puts("FM");
    #endif 
    wert |= (1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 3)
  {
    // 2 AM
    #ifdef debug 
    uart_puts("AM"); 
    #endif
    wert |= (1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 0)
  {
    // 3 USB
    #ifdef debug 
    uart_puts("USB");
    #endif
    wert |= (1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 1)
  {
    // 4 LSB
    #ifdef debug 
    uart_puts("LSB");
    #endif
    wert |= (1 << TREIBER_LSB);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_USB);
    //
    // LSB -5kHz!
    freq2tune=freq2tune-5000;
  }
  else
  {
    #ifdef debug 
    uart_puts("unbekannte modulation!");
    #endif
  }
  #ifdef debug 
  uart_puts("\r\n");
  #endif
  treiber(wert);

  #ifdef debug
  uart_puts("tune: Frequenz : ");
  uart_puts(ltoa(freq2tune, string, 10));
  uart_puts("\r\n");
  #endif
  //
  // Maybe it should be better ? The port expander sends some "stuff"... 
  //cli();
  begin1();
  //
  // Festlegung Kanalraster
  // Referenzquarz ist 10240kHz
  // 10240 / 2048 = 5, also der Teiler von 2048 waere dann ein Kanalraster von 5khz
  // 10240 / step = teiler
  // die Bitfolge muss 17 Bits lang sein
  unsigned int teiler_ref; 
  // TODO: define !
  teiler_ref=10240000/(step2tune*1000);
  #ifdef debug
  uart_puts("tune: Teiler REF : ");
  uart_puts(ltoa(teiler_ref, string, 10));
  uart_puts("\r\n");
  #endif

  int index[16];
  int i;
  for(i=0; teiler_ref > 0; i++)
  {
    index[i]=teiler_ref%2;
    teiler_ref=teiler_ref/2;
  }
  for(;i < 16; i++)
  {
    index[i]=0;
  }
  for (; i > 0; i--)
  {
    if(index[i-1] == 0)
    {
      data0();
    }
    else
    {
      data1();
    }
  }
  //
  // Achtung, Abschlussbit!
  data1();
  end1();
  //
  // Festlegen des Teilers fuer die andere Seite
  // N Paket:
  // stellt den Teiler von der Sollfrequenz ein. 
  // (27255 + 10695) / 7590 = 5
  begin1();
  unsigned int teiler_soll;
  // TODO! / 1000 !! ??
  unsigned long teiler_soll_temp=(freq2tune)+10695000;
  teiler_soll=teiler_soll_temp/(step2tune*1000);
  #ifdef debug
  uart_puts("tune: Teiler SOLL : ");
  uart_puts(ltoa(teiler_soll, string, 10));
  uart_puts("\r\n");
  #endif

  int index_soll[24];
  int j;
  for(j=0; teiler_soll > 0; j++)
  {
    index_soll[j]=teiler_soll%2;
    teiler_soll=teiler_soll/2;
  }
  for(;j < 24; j++)
  {
    index_soll[j]=0;
  }
  for (; j > 0; j--)
  {
    if(index_soll[j-1] == 0)
    {
      data0();
    }
    else
    {
      data1();
    }
  }
  // 
  // Achtung, Abschlussbit!
  data0();
  end1();
	
  //display_write_frequenz(freq2tune);
  /*
  if(modus==1)
  {
    freq=freq2tune;
  }
  */
  #ifdef debug
  uart_puts("tune(): Ende\r\n");
  #endif
  return 0; 
}
/*
void set_modulation(unsigned int mod)
{
  #ifdef debug 
  uart_puts("Modulation: ");
  #endif
  if(mod == 2)
  {
    // 1 FM
    #ifdef debug 
    uart_puts("FM");
    #endif 
    wert |= (1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 3)
  {
    // 2 AM
    #ifdef debug 
    uart_puts("AM"); 
    #endif
    wert |= (1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 0)
  {
    // 3 USB
    #ifdef debug 
    uart_puts("USB");
    #endif
    wert |= (1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 1)
  {
    // 4 LSB
    #ifdef debug 
    uart_puts("LSB");
    #endif
    wert |= (1 << TREIBER_LSB);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_USB);
  }
  else
  {
    #ifdef debug 
    uart_puts("unbekannte modulation!");
    #endif
  }
  #ifdef debug 
  uart_puts("\r\n");
  #endif
  treiber(wert);
  display_write_mod(mod);
}
*/

void i2c_poti(unsigned char value)
{
  i2c_init();
  /*
  i2c_start_wait(0x50);
  i2c_write(0x8);
  i2c_write(0x80);
  i2c_stop();
  */

  i2c_start_wait(0x50);
  i2c_write(0x0);
  i2c_write(value);
  i2c_stop();
}


int rogerbeep(void)
{
  #ifdef debug
  uart_puts("rogerbeep()\r\n");
  #endif
/*
  init_tone();
  tone(2520);
  _delay_ms(250);
  tone(0);
  return 0;
*/
  init_tone();
  i2c_poti(0x0);
  tone(2520);
  _delay_ms(1000);
  i2c_poti(0xff);
  tone(2520);
  _delay_ms(1000);
  tone(0);
  return(0);
}
void init_tone(void)
{
  //
  // Anregung: http://www.mikrocontroller.net/topic/215420
  DDRB |= (1<<PB5);

  TCCR1A |= (1 << WGM11) | (1<<COM1A1);
  // 64 ist Vorteiler
  TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS10) | (1 << CS11);
}

//
// TODO: Tiefe Frequenzen passen nicht!
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
