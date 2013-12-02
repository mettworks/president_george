#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "transceiver.h"
#include "3wire.h"
#include "display.h"
#include "memory.h"
#include "operating.h"
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif




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

#define HAM_FREQ_MIN 28000
//#define HAM_FREQ_MAX 28050
#define HAM_FREQ_MAX 29690

#define CB_CH_MIN 1
#define CB_CH_MAX 3

unsigned int wert = 0;
int ichsende=0;

extern unsigned int freq;
extern unsigned int step;
extern unsigned int cb_channel;
extern unsigned int cb_mod;
extern int mod;
extern int modus;

char string[10];

extern unsigned int memory[MEM_SIZE];

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
	uart_puts("init_geraet()\r\n");
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
	  // PA4
  DDRA |= (1<<PA4);	// Bitbanging SPI
  // PA6
  DDRA |= (1<<PA6);	// Bitbanging SPI
  // PA5
  DDRA |= (1<<PA5);	// Bitbanging SPI
  // PA3
  DDRA |= (1<<PA3);	// Bitbanging SPI
	PORTA &= ~(1<<PA3);	
	PORTA &= ~(1<<PA4);	
	PORTA &= ~(1<<PA5);
	PORTA &= ~(1<<PA6);
	
	read_memory();
	freq = memory[3] + (memory[2] << 8);
	mod=memory[5];
	modus=memory[1];
	step=5;
	cb_channel=memory[4];
	cb_mod=memory[6];
	if((26565 > freq) || (29690 < freq) || (freq == 0))
	{
		#ifdef debug
		uart_puts("Frequenz aus dem EEPROM ist falsch!\r\n");
		#endif
		freq=28000;
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
		//tune(freq,step);
		//_delay_ms(28);
		//modulation(mod);
	}
	else
	{
		//channel(cb_channel);
		//_delay_ms(28);
		//modulation(cb_mod);
	}
	setmodus(modus);
	uart_puts("Frequenz: ");
	uart_puts(itoa(freq, string, 10));
	uart_puts("\r\n");
	return 0;
}

int tx(void)
{
	if(ichsende != 1)
	{
		ichsende=1;
		// alle Bits sind in der gleichen Reihenfolge wie im Schaltplan angegeben
		//
		// Senden:
	  // 0100 0001  0100 0000
		// Pause, 7ms
		// 0110 0001  0100 0000
		wert &= ~(1 << TREIBER_MUTE);
		treiber(wert);
		_delay_ms(7);
		wert |= (1 << TREIBER_TR);
		treiber(wert);
		_delay_ms(250);
		beep();
	}
	return 0;
}

int rx(void)
{
	ichsende=0;
	// alle Bits sind in der gleichen Reihenfolge wie im Schaltplan angegeben
	//
	// Empfangen:
	// 0100 0001  0100 0000
	// Pause, 4ms
	// 0100 1001  0100 0000
	// Pause, 4ms
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
	unsigned int data=0;
	if(ch == 1)
	{
		data=26965;
	}
	else if(ch==2)
	{
		data=26975;
	}
	else if(ch==3)
	{
		data=26985;
	}	
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
		uart_puts("channel(): Kanal ist groesser als CB_CH_MAX -> CB_CH_MIN ");
	}
	if(ch < CB_CH_MIN)
	{
		ch=CB_CH_MAX;
		uart_puts("channel(): Kanal ist kleiner als CB_CH_MIN -> CB_CH_MAX ");
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

int tune(unsigned int freq2tune,unsigned int step2tune)
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
	uart_puts("tune(): Beginn\r\n");
  //
	// hier müssen Interrupts gesperrt werden!
	// der Port Expander schickt noch Quatsch weil der Taster prellt, dann wird irgendwo abgebrochen
	cli();
  begin1();
  //
  // Festlegung Kanalraster
  // Referenzquarz ist 10240kHz
  // 10240 / 2048 = 5, also der Teiler von 2048 waere dann ein Kanalraster von 5khz
  // 10240 / step = teiler
  // die Bitfolge muss 17 Bits lang sein
	/*
  #ifdef debug 
  uart_puts("tune(): Frequenz ");
  char text[10];
  utoa(freq2tune,text,10);
  uart_puts(text);
  uart_puts("kHz\r\n");
	#endif
	*/
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
	
	uart_puts("tune(): Ende 1. Packet\r\ntune(): Beginn 2. Packet\r\n");

	
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
  unsigned int teiler_soll_temp=freq2tune+10695;
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
	
	memory[2] = freq2tune / 256;
	memory[3] = freq2tune % 256;
	if(modus==1)
	{
		freq=freq2tune;
	}
	
	//sei();
	uart_puts("tune(): Ende\r\n");
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