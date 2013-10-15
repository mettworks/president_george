
#define F_CPU 18432000UL
//#define BAUD 9600UL
#define debug

#include <avr/io.h>
#include <util/delay.h>
//#include <util/setbaud.h> 
//#include <avr/interrupt.h>
#include <i2cmaster.h>
//#include "eeprom.h"
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

unsigned char segmente[6];
// fuer debugausgaben
char Buffer[7];	

//
// immer nur eine Zahl, also 0 bis 9
display_convert_number(unsigned char number,unsigned char segmente[6])
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

display_write_modus(unsigned char modus)
{
	uart_puts("Beginn display_write_modus()");
	uart_puts("\r\n");
	// RX 3
	// TX 1
	if(modus == 0)
	{
		uart_puts("RX");
		daten[3]=0x01;
		daten[1]=0x0;
	}
	else
	{
		uart_puts("TX");
		daten[3]=0x0;
		daten[1]=0x01;
	}
	uart_puts("\r\n");
	display_send();
}

display_write_mod(unsigned char mod)
{
	uart_puts("Beginn display_write_mod()");
	uart_puts("\r\n");

	// 1 FM 21
	// 2 AM 12
	// 3 USB 39
	// 4 LSB 30
	
	if(mod == 1)
	{
		uart_puts("FM");
		daten[12]=0x0;
		daten[21]=0x01;
		daten[30]=0x0;
		daten[39]=0x0;
	}
	else if(mod == 2)
	{
		uart_puts("AM");
		daten[12]=0x01;
		daten[21]=0x0;
		daten[30]=0x0;
		daten[39]=0x0;
	}
	else if(mod == 3)
	{
		uart_puts("USB");	
		daten[12]=0x0;
		daten[21]=0x0;
		daten[30]=0x0;
		daten[39]=0x01;
	}
	else if(mod == 4)
	{
		uart_puts("LSB");
		daten[12]=0x0;
		daten[21]=0x0;
		daten[30]=0x01;
		daten[39]=0x0;
	}
	uart_puts("\r\n");
	display_send();
}

display_write_frequenz(unsigned int frequenz)
{
	uart_puts("Beginn display_write_frequenz()");
	uart_puts("\r\n");
	itoa(frequenz, Buffer, 10);
	uart_puts(" frequenz: ");
	uart_puts(Buffer);
	uart_puts("\r\n");

	/*
		1. Ziffer
		a 24
		b 27
		c 28
		d 26
		e 23
		f 22
		g 25
	
		2. Ziffer
		a	33
		b 36
		c 37
		d 35
		e	32
		f 31
		g 34
		
		PUNKT ist die 38
		
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
	daten[38]=0x01;

	// 
	// zerlegen
	unsigned char x;
	
	x = frequenz / 10000;
	itoa(x, Buffer, 10);
	uart_puts(" 1. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	segmente[0]=24;
	segmente[1]=27;
	segmente[2]=28;
	segmente[3]=26;
	segmente[4]=23;
	segmente[5]=22;
	segmente[6]=25;
	display_convert_number(x,segmente);
	
	x = frequenz % 10000 / 1000;
	itoa(x, Buffer, 10);
	uart_puts(" 2. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	segmente[0]=33;
	segmente[1]=36;
	segmente[2]=37;
	segmente[3]=35;
	segmente[4]=32;
	segmente[5]=31;
	segmente[6]=34;
	display_convert_number(x,segmente);
	
	x = frequenz % 1000 / 100;
	itoa(x, Buffer, 10);
	uart_puts(" 3. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	segmente[0]=42;
	segmente[1]=45;
	segmente[2]=46;
	segmente[3]=44;
	segmente[4]=41;
	segmente[5]=40;
	segmente[6]=43;
	display_convert_number(x,segmente);
	
	x = frequenz % 100 / 10;
	itoa(x, Buffer, 10);
	uart_puts(" 2. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	segmente[0]=51;
	segmente[1]=54;
	segmente[2]=55;
	segmente[3]=53;
	segmente[4]=50;
	segmente[5]=49;
	segmente[6]=52;
	display_convert_number(x,segmente);
	
	x = frequenz % 10;
	itoa(x, Buffer, 10);
	uart_puts(" 1. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	segmente[0]=60;
	segmente[1]=63;
	segmente[2]=64;
	segmente[3]=62;
	segmente[4]=59;
	segmente[5]=58;
	segmente[6]=61;
	display_convert_number(x,segmente);
	
	display_send();
}

display_write_channel(unsigned char channel)
{
	uart_puts("Beginn display_write_channel()");
	uart_puts("\r\n");
	itoa(channel, Buffer, 10);
	uart_puts(" channel: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
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
	itoa(x, Buffer, 10);
	uart_puts(" 1. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	segmente[0]=69;
	segmente[1]=72;
	segmente[2]=73;
	segmente[3]=71;
	segmente[4]=67;
	segmente[5]=66;
	segmente[6]=70;
	display_convert_number(x,segmente);
	
	x = channel % 10;
	itoa(x, Buffer, 10);
	uart_puts(" 2. Stelle: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
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

display_init()
{
	uart_puts("Beginn display_init():");
	uart_puts("\r\n");
	i2c_start_wait(0x70);
	i2c_write(0xe0);     			// Device Select 0
  i2c_write(0xcf);	  	// multiplex 1100 1111 , 3 BP's
  i2c_write(0xF8);   				// 1111 1000 = immer die 1. RAM Bank (macht eh keinen Sinn...)
  i2c_write(0xF0);   				// 1111 0000 = Blinken, alles abgeschaltet 
	i2c_stop();
	
}
display_send()
{
	display_init();
	// die Daten die dann wirklich über den i2c Bus gehen
	unsigned char daten2send;
	char bits[7];
	int y,addr;
	uart_puts("Beginn display_send():");
	uart_puts("\r\n");
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
		y=y+6;
		addr=addr+2;
	}
}
