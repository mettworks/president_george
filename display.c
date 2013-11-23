
#define F_CPU 18432000UL
//#define BAUD 9600UL
//#define debug

#include <avr/io.h>
#include <util/delay.h>
//#include <util/setbaud.h> 
//#include <avr/interrupt.h>
#include "i2c.h"
#include "display.h"
//#include "eeprom.h"
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif
//
// Das Display wird erstmal mit 4BP's betrieben, mit 3 gab es Probleme... :-(
// Nach meinem Verständniss sollte der Init Kram nicht immer mitgesendet werden?
// -> Sonst wird das 1. DatenBit nicht richtig gesetzt, sehr seltsam.
//
//
// Speicherorganisation:
// waagerecht sind die Segmente
// senkrecht sind die Backplanes
// Bytes werden von oben nach unten geschrieben
//
// 		0 1 2 ... 37 38 39
//	0
//	1
//	2
//	3
// 
// Ablauf (3 BP's)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1. Lauf Übertragung in den RAM des Display Controllers:
//
// Startadresse 0
//
//   | 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 29
// -----------------------------------------------------------------------------------------------------------------
// 0 | 7 4 7 4
// 1 | 6 3 6 3 
// 2 | 5 2 5 2
// 3 |
//
// 
// pro Bit gibt es aus "Einfachheitsgründen" ein Byte 96
unsigned char daten[104];
//

unsigned char segmente[7];
// fuer debugausgaben
char Buffer[7];	

//
// immer nur eine Zahl, also 0 bis 9
void display_convert_number(unsigned char number,unsigned char segmente[6])
{
	if(number == 0)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x01;
		daten[segmente[5]]=0x01;
		daten[segmente[6]]=0x0;
	}
	if(number == 1)
	{
		daten[segmente[0]]=0x0;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x0;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x0;
		daten[segmente[6]]=0x0;
	}
	if(number == 2)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x0;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x01;
		daten[segmente[5]]=0x0;
		daten[segmente[6]]=0x01;
	}
	if(number == 3)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x0;
		daten[segmente[6]]=0x01;
	}
	if(number == 4)
	{
		daten[segmente[0]]=0x0;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x0;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x01;
		daten[segmente[6]]=0x01;
	}
	if(number == 5)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x0;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x01;
		daten[segmente[6]]=0x01;
	}
	if(number == 6)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x0;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x01;
		daten[segmente[5]]=0x01;
		daten[segmente[6]]=0x01;
	}
	if(number == 7)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x0;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x0;
		daten[segmente[6]]=0x0;
	}
	if(number == 8)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x01;
		daten[segmente[5]]=0x01;
		daten[segmente[6]]=0x01;
	}
	if(number == 9)
	{
		daten[segmente[0]]=0x01;
		daten[segmente[1]]=0x01;
		daten[segmente[2]]=0x01;
		daten[segmente[3]]=0x01;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x01;
		daten[segmente[6]]=0x01;
	}
}

void display_write_modus(unsigned char modus)
{
	#ifdef debug
	uart_puts("Beginn display_write_modus()");
	uart_puts("\r\n");
	#endif
	// RX 3
	// TX 1
	if(modus == 0)
	{
		#ifdef debug
		uart_puts("RX");
		#endif
		daten[3]=0x01;
		daten[2]=0x0;
	}
	else
	{
		#ifdef debug
		uart_puts("TX");
		#endif
		daten[3]=0x0;
		daten[2]=0x01;
	}
	#ifdef debug
	uart_puts("\r\n");
	#endif
	display_send();
}

void display_write_mod(unsigned char mod)
{
	#ifdef debug
	uart_puts("Beginn display_write_mod()");
	uart_puts("\r\n");
	#endif 

	// 1 FM 21
	// 2 AM 12
	// 3 USB 39
	// 4 LSB 30
	
	if(mod == 1)
	{
		#ifdef debug
		uart_puts("FM");
		#endif
		daten[12]=0x0;
		daten[21]=0x01;
		daten[30]=0x0;
		daten[39]=0x0;
	}
	else if(mod == 2)
	{
		#ifdef debug
		uart_puts("AM");
		#endif
		daten[12]=0x01;
		daten[21]=0x0;
		daten[30]=0x0;
		daten[39]=0x0;
	}
	else if(mod == 3)
	{
		#ifdef debug
		uart_puts("USB");	
		#endif
		daten[12]=0x0;
		daten[21]=0x0;
		daten[30]=0x0;
		daten[39]=0x01;
	}
	else if(mod == 4)
	{
		#ifdef debug
		uart_puts("LSB");
		#endif
		daten[12]=0x0;
		daten[21]=0x0;
		daten[30]=0x01;
		daten[39]=0x0;
	}
	#ifdef debug
	uart_puts("\r\n");
	#endif
	display_send();
}

void display_write_frequenz(unsigned int frequenz)
{

/*
		daten[33]=0x01;
		daten[36]=0x01;
		daten[38]=0x01;
		daten[34]=0x01;
		daten[31]=0x01;
		daten[32]=0x01;
		daten[35]=0x01;
	display_send();
	while(1)
	{
	}
*/
	#ifdef debug
	uart_puts("Beginn display_write_frequenz()");
	uart_puts("\r\n");
	itoa(frequenz, Buffer, 10);
	uart_puts(" frequenz: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	/*
		1. Ziffer
		a 24
		b 27
		c 29
		d 25
		e 22
		f 23
		g 26
	
		2. Ziffer
		a	33
		b 36
		c 38
		d 35
		e	31
		f 32
		g 35
		
		PUNKT ist die 37
		
		3. Ziffer
		a 42
		b 45
		c 46
		d 44
		e 41
		f 40
		g 43
		
		4. Ziffer
		a	51
		b 54
		c 55
		d	53
		e	50
		f	49
		g	52
		
		5. Ziffer
		a	60
		b	63
		c	64
		d	62
		e	59
		f	58
		g	61
		
	*/

	// Punkt anmachen
	daten[37]=0x01;

	// 
	// zerlegen
	unsigned char x;
	
	x = frequenz / 10000;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 1. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=24;
	segmente[1]=27;
	segmente[2]=29;
	segmente[3]=25;
	segmente[4]=22;
	segmente[5]=23;
	segmente[6]=26;
	display_convert_number(x,segmente);
	
	x = frequenz % 10000 / 1000;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 2. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=33;
	segmente[1]=36;
	segmente[2]=38;
	segmente[3]=34;
	segmente[4]=31;
	segmente[5]=32;
	segmente[6]=35;
	display_convert_number(x,segmente);
	
	x = frequenz % 1000 / 100;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 3. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=42;
	segmente[1]=45;
	segmente[2]=47;
	segmente[3]=43;
	segmente[4]=40;
	segmente[5]=41;
	segmente[6]=44;
	display_convert_number(x,segmente);
	
	x = frequenz % 100 / 10;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 2. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=51;
	segmente[1]=54;
	segmente[2]=56;
	segmente[3]=52;
	segmente[4]=49;
	segmente[5]=50;
	segmente[6]=53;
	display_convert_number(x,segmente);
	
	x = frequenz % 10;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 1. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=60;
	segmente[1]=63;
	segmente[2]=65;
	segmente[3]=61;
	segmente[4]=58;
	segmente[5]=59;
	segmente[6]=62;
	display_convert_number(x,segmente);
	
	display_send();
}

void display_write_channel(unsigned char channel)
{
	#ifdef debug
	uart_puts("Beginn display_write_channel()");
	uart_puts("\r\n");
	itoa(channel, Buffer, 10);
	uart_puts(" channel: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	/* 1. Ziffer:
		a	69
		b	72
		c	73
		d	71
		e	67
		f	66
		g	70

		2. Ziffer:
		a 78
		b 81
		c 82
		d 80
		e 76
		f	75
		g 79
	*/
	
	// 
	// zerlegen
	unsigned char x;
	
	x = channel / 10;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 1. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=69;
	segmente[1]=72;
	segmente[2]=73;
	segmente[3]=71;
	segmente[4]=67;
	segmente[5]=66;
	segmente[6]=70;
	display_convert_number(x,segmente);
	
	x = channel % 10;
	#ifdef debug
	itoa(x, Buffer, 10);
	uart_puts(" 2. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	#endif
	segmente[0]=78;
	segmente[1]=81;
	segmente[2]=82;
	segmente[3]=80;
	segmente[4]=76;
	segmente[5]=75;
	segmente[6]=79;
	display_convert_number(x,segmente);
	
	display_send();
}

void display_init(void)
{
	#ifdef debug
	uart_puts("Beginn display_init():");
	uart_puts("\r\n");
	#endif
	i2c_start_wait(0x70);
	i2c_write(0xe0);     			// Device Select 0
  i2c_write(0xcf);	  	// multiplex 1100 1111 , 3 BP's
  i2c_write(0xF8);   				// 1111 1000 = immer die 1. RAM Bank (macht eh keinen Sinn...)
  i2c_write(0xF0);   				// 1111 0000 = Blinken, alles abgeschaltet 
	i2c_stop();
	#ifdef debug
	uart_puts("Ende display_init():");
	uart_puts("\r\n");
	#endif
}

void display_send(void)
{
	display_init();
	// die Daten die dann wirklich über den i2c Bus gehen
	unsigned char daten2send=0;
	//char bits[7];
	int y,addr;
	#ifdef debug
	uart_puts("Beginn display_send():");
	uart_puts("\r\n");
	#endif
	y=0;
	addr=0;

	for ( y = 0; y < 98; )
	{
		if(daten[y] & (1<<0x00)) 
		{
			daten2send |= (1<<0x07);
		}
		else
		{
			daten2send &= ~(1<<0x07);
		}
		
		if(daten[y+1] & (1<<0x00)) 
		{
			daten2send |= (1<<0x06);
		}
		else
		{
			daten2send &= ~(1<<0x06);
		}
		if(daten[y+2] & (1<<0x00)) 
		{
			daten2send |= (1<<0x05);
		}
		else
		{
			daten2send &= ~(1<<0x05);
		}
		
		if(daten[y+3] & (1<<0x00)) 
		{
			daten2send |= (1<<0x04);
		}
		else
		{
			daten2send &= ~(1<<0x04);
		}

		if(daten[y+4] & (1<<0x00)) 
		{
			daten2send |= (1<<0x03);
		}
		else
		{
			daten2send &= ~(1<<0x03);
		}
		if(daten[y+5] & (1<<0x00)) 
		{
			daten2send |= (1<<0x02);
		}
		else
		{
			daten2send &= ~(1<<0x02);
		}				
		if(daten[y+6] & (1<<0x00)) 
		{
			daten2send |= (1<<0x01);
		}
		else
		{
			daten2send &= ~(1<<0x01);
		}				
		if(daten[y+7] & (1<<0x00)) 
		{
			daten2send |= (1<<0x00);
		}
		else
		{
			daten2send &= ~(1<<0x00);
		}				

		i2c_start_wait(0x70);
		i2c_write(addr);
		i2c_write(daten2send);
		i2c_stop();
		#ifdef debug
		itoa(addr, Buffer, 10);
		uart_puts(" addr: ");
		uart_puts(Buffer);
		itoa(y, Buffer, 10);
		uart_puts(" y: ");
		uart_puts(Buffer);
		itoa(daten2send, Buffer, 10);
		uart_puts(" daten2send: ");
		uart_puts(Buffer);
		uart_puts("\r\n");
		#endif
		y=y+6;
		addr=addr+2;
	}
}
