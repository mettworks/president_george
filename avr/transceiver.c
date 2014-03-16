#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "transceiver.h"
#include "3wire.h"
#include "display.h"
#include "memory.h"
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

#define HAM_FREQ_MIN 28000000
#define HAM_FREQ_MAX 29690000

#define CB_CH_MIN 1
#define CB_CH_MAX 80

unsigned int wert = 0;
int ichsende=0;
extern unsigned int freq;
extern unsigned int step;
extern unsigned int cb_channel;
extern unsigned int cb_mod;
extern int mod;
extern int modus;
extern unsigned int ctcss;
extern unsigned int rpt;
extern unsigned int echo_ham;
extern unsigned int beep_ham;
char string[10];

extern unsigned int memory[MEM_SIZE];
extern unsigned long freq_a;
extern unsigned long freq_b;
extern char vfo;

void off(void)
{
  cli();
  #ifdef debug
  uart_puts("AUS\r\n");
  #endif
  save2memory();
  while(1)
  {
    _delay_ms(1000);
    #ifdef debug
    uart_puts("1 Sekunde\r\n");
    #endif
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
Mod HAM A   Mod HAM B     Mod CB  color display green   color keys green 
                                  color display red     color keys red
Byte 13
#######
     7    6     5           4               3               2               1             0
1    on   cb    split on    NBANL ham on    NBANL cb on     HICUT ham on    HICUT cb on   VFO B active
0    off  ham   split off   NBANL ham off   NBANL cb off    HICUT ham off   HICUT cb off  VFO A active

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

  freq_a = ((unsigned long int) memory[0]) + ((unsigned long int) memory[1] << 8) + ((unsigned long int) memory[2] << 16) + ((unsigned long int) memory[3] << 24);
  freq_b = ((unsigned long int) memory[4]) + ((unsigned long int) memory[5] << 8) + ((unsigned long int) memory[6] << 16) + ((unsigned long int) memory[7] << 24);

  if(memory[13] & 0x0) 
  {
    uart_puts("VFO A ausgelesen\r\n");
    vfo='A';
  }
  else
  {
    uart_puts("VFO B ausgelesen\r\n");
    vfo='B';
  }

  mod=memory[5];
  modus=memory[1];
  step=5;
  cb_channel=memory[4];
  cb_mod=memory[6];
  ctcss=memory[16];
  rpt=memory[18];
  echo_ham=memory[12];
  beep_ham=memory[14];
  //ctcss_tone=memory[17];
  if((26565000 > freq_a) || (29690000 < freq_a) || (freq_a == 0))
  {
    #ifdef debug
    uart_puts("Frequenz A aus dem EEPROM ist falsch!\r\n");
    #endif
    freq_a=28000000;
  }
  if((26565000 > freq_b) || (29690000 < freq_b) || (freq_b == 0))
  {
    #ifdef debug
    uart_puts("Frequenz B aus dem EEPROM ist falsch!\r\n");
    #endif
    freq_b=28000000;
  }
  if((1 > mod) || (4 < mod))
  {
    #ifdef debug
    uart_puts("Modulationsart aus dem EEPROM ist falsch!\r\n");
    #endif
    mod=1;
  }
  if((1 > modus) || (2 < modus))
  {
    #ifdef debug
    uart_puts("Modus aus dem EEPROM ist falsch!\r\n");
    #endif
    modus=1;
  }
  if((1 > cb_channel) || (4 < cb_channel))
  {
    #ifdef debug
    uart_puts("Kanal (CB) aus dem EEPROM ist falsch!\r\n");
    #endif
    cb_channel=1;
    memory[4]=1;
  }	
  if((1 > cb_mod) || (4 < cb_mod))
  {
    #ifdef debug
    uart_puts("Modulationsart (CB) aus dem EEPROM ist falsch!\r\n");
    #endif
    cb_mod=1;
    memory[6]=1;
  }

  treiber(wert); 
  _delay_ms(28);
  wert |= (1 << TREIBER_MUTE);
  treiber(wert);
  _delay_ms(28);
  if(modus == 1)
  {
    #ifdef debug
    uart_puts("Modus: HAM\r\n");
    #endif
    tune(freq_a,step);
    _delay_ms(28);
    modulation(mod);
  }
  else
  {
    #ifdef debug
    uart_puts("Modus: CB\r\n");
    #endif
    channel(cb_channel);
    _delay_ms(28);
    modulation(cb_mod);
  }
  set_ctcss(ctcss);
  set_rpt(rpt);
  set_echo(echo_ham);
  set_beep(beep_ham);
  //display_write_vfo('A');
  setvfo();
  display_write_modus(0);

  
  #ifdef debug
  uart_puts("Frequenz: ");
  uart_puts(itoa(freq_a, string, 10));
  uart_puts("\r\n");
  uart_puts("init_geraet() ENDE\r\n");
  #endif
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
    echo_ham=1;
    memory[12]=1;
  }
  else
  {
    #ifdef debug
    uart_puts("set_echo(): AUS\r\n");
    #endif
    wert &= ~(1 << TREIBER_ECHO);
    treiber(wert);
    display_echo(0);
    echo_ham=0;
    memory[12]=0;
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
    beep_ham=1;
    memory[14]=1;
  }
  else
  {
    #ifdef debug
    uart_puts("set_beep(): AUS\r\n");
    #endif
    display_beep(0);
    beep_ham=0;
    memory[14]=0;
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
    memory[18]=1;
  }
  else
  {
    #ifdef debug
    uart_puts("set_rpt(): AUS\r\n");
    #endif
    display_rpt(0);
    rpt=0;
    memory[18]=0;
  }
}

int tx(void)
{
  if(ichsende != 1)
  {
    ichsende=1;
    if(rpt==1)
    {
      tune(freq-100,5);
    }
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
    if(ctcss==1)
    {
      tone(67);
    }
    //beep();
  }
  return 0;
}

int rx(void)
{
  if(rpt==1)
  {
    tune(freq+100,5);
  }
  if(ctcss==1)
  {
    tone(0);
  }
  if(beep_ham == 1)
  {
    rogerbeep();
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
 
int ch2freq(unsigned int ch)
{

  #ifdef debug 
  uart_puts("ch2freq(): Kanal ");
  char text[10];
  utoa(ch,text,10);
  uart_puts(text);
  uart_puts(" -> ");
  #endif
  unsigned int data;
  data=channels[ch-1];
  #ifdef debug
  utoa(data,text,10);
  uart_puts(text);
  uart_puts("\r\n");
  #endif
  return data;
}

void channel(unsigned int ch)
{
  if(ch > CB_CH_MAX)
  {
    ch=CB_CH_MIN;
    #ifdef debug
    uart_puts("channel(): Kanal ist groesser als CB_CH_MAX -> CB_CH_MIN ");
    #endif
  }
  if(ch < CB_CH_MIN)
  {
    ch=CB_CH_MAX;
    #ifdef debug
    uart_puts("channel(): Kanal ist kleiner als CB_CH_MIN -> CB_CH_MAX ");
    #endif
  }
  unsigned int cb_freq;
  #ifdef debug 
  uart_puts("channel(): Kanal ");
  char text[10];
  utoa(ch,text,10);
  uart_puts(text);
  uart_puts("\r\n");
  #endif
  cb_freq=ch2freq(ch);
  tune(cb_freq,5);
  display_write_channel(ch);
  memory[4]=ch;
  cb_channel=ch;
}

int tune(unsigned long freq2tune,unsigned int step2tune)
{
  if(modus==1)
  {
    if(freq2tune > HAM_FREQ_MAX)
    {
      freq2tune=HAM_FREQ_MIN;
    }
    if(freq2tune < HAM_FREQ_MIN)
    {
      freq2tune=HAM_FREQ_MAX;
    }
  }

  //freq2tune=freq2tune/1000;
  #ifdef debug
  uart_puts("tune(): Beginn\r\n");
  uart_puts("tune: Frequenz : ");
  uart_puts(ltoa(freq_a, string, 10));
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
  teiler_ref=10240/step2tune;
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
  unsigned int teiler_soll_temp=(freq2tune/1000)+10695;
  teiler_soll=teiler_soll_temp/step2tune;
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
	
  display_write_frequenz(freq2tune);
  //
  // Frequenz erfolgreich geändert, ab in EEPROM, bei Spannungswegfall... :-)
  if(vfo == 'A')
  {
    memory[3] = freq2tune / 16777215;
    memory[2] = freq2tune / 65535;
    memory[1] = freq2tune / 256;
    memory[0] = freq2tune % 256;
  }
  else
  {
    memory[7] = freq2tune / 16777215;
    memory[6] = freq2tune / 65535;
    memory[5] = freq2tune / 256;
    memory[4] = freq2tune % 256;
  }  
  if(modus==1)
  {
    freq=freq2tune;
  }

  #ifdef debug
  uart_puts("tune(): Ende\r\n");
  #endif
  return 0;
}

int modulation(unsigned int mod)
{
  #ifdef debug 
  uart_puts("Modulation: ");
  #endif
  if(mod == 1)
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
  else if(mod == 2)
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
  else if(mod == 3)
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
  else if(mod == 4)
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
  //
  // ab ins EEPROM
  if(modus==1)
  {
    memory[5]=mod;
  }
  else
  {	
    memory[6]=mod;
  }
  return 0;
}

int beep(void)
{
/*
  // 2 Töne
  DDRB |= (1<<PB5); 

  TCCR1A = (1<<WGM10) | (1<<COM1A1); 
  TCCR1B = (1<<CS11) | (1<<CS10);
  //TCCR1B=
  OCR1A = 128-1;
*/
/* 
  _delay_ms(250);
  TCCR1A &= ~((1 << COM1A1) | (1 << WGM10)); 
  _delay_ms(100);
  TCCR1A = (1<<WGM10) | (1<<COM1A1); 
  _delay_ms(250);
  TCCR1A &= ~((1 << COM1A1) | (1 << WGM10));
  */
  return 0;
}

int rogerbeep(void)
{
  #ifdef debug
  uart_puts("rogerbeep()\r\n");
  #endif
  tone(1000);
  _delay_ms(500);
  tone(1500);
  _delay_ms(500);
  tone(0);
/*
  //
  // 2 Töne
  DDRB |= (1<<PB5); 
  TCCR1A = (1<<WGM10) | (1<<COM1A1); 
  TCCR1B = (1<<CS11) | (1<<CS10);	
  OCR1A = 128-1; 
  
  _delay_ms(1000);

  TCCR1A &= ~((1 << COM1A1) | (1 << WGM10)); 
  _delay_ms(100);
  
  TCCR1A = (1<<WGM10) | (1<<COM1A1); 
  _delay_ms(250);
  	
  TCCR1A &= ~((1 << COM1A1) | (1 << WGM10));
*/
  return 0;
}
