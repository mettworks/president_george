#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "transceiver.h"
#include "3wire.h"
#include "display.h"
#include "memory.h"
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

unsigned int wert = 0;
int ichsende=0;

extern unsigned int freq;
extern unsigned int step;
extern int mod;

extern unsigned int memory[6];

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
	freq = memory[1] + (memory[0] << 8);
	freq=28230;
	mod=memory[2];
	
	if(freq == 0)
	{
		#ifdef debug
		uart_puts("ICH HABE ALZHEIMER!\r\n");
		#endif
		freq=27000;
	}
	
	treiber(wert); 
	_delay_ms(28);
	wert |= (1 << TREIBER_MUTE);
	treiber(wert);
	_delay_ms(28);
	tune(freq,step);
	_delay_ms(28);
	modulation(mod);
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

int tune(unsigned int freq2tune,unsigned int step2tune)
{
  //
	// hier m�ssen Interrupts gesperrt werden!
	// der Port Expander schickt noch Quatsch weil der Taster prellt, dann wird irgendwo abgebrochen
	cli();
  begin1();
  //
  // Festlegung Kanalraster
  // Referenzquarz ist 10240kHz
  // 10240 / 2048 = 5, also der Teiler von 2048 waere dann ein Kanalraster von 5khz
  // 10240 / step = teiler
  // die Bitfolge muss 17 Bits lang sein
	
  #ifdef debug 
  uart_puts("tune(): Frequenz ");
  char text[10];
  utoa(freq,text,10);
  uart_puts(text);
  uart_puts("kHz\r\n");
	#endif
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
	// Frequenz erfolgreich ge�ndert, ab in EEPROM, bei Spannungswegfall... :-)
	memory[0] = freq2tune / 256;
	memory[1] = freq2tune % 256;
	sei();
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
	memory[2]=mod;
	return 0;
}

int rogerbeep(void)
{
	//
	// 2 T�ne
	DDRB |= (1<<PB5); 
	TCCR1A = (1<<WGM10) | (1<<COM1A1); 
	TCCR1B = (1<<CS11) | (1<<CS10);
	OCR1A = 128-1; 
	_delay_ms(250);
	TCCR1A &= ~((1 << COM1A1) | (1 << WGM10)); 
	_delay_ms(100);
	TCCR1A = (1<<WGM10) | (1<<COM1A1); 
	_delay_ms(250);
	TCCR1A &= ~((1 << COM1A1) | (1 << WGM10));
	return 0;
}