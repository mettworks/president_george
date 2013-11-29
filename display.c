
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

/*

87 BEEP
89 ECHO
90 NB/ANL
94 Pfeil rechts unten ??
96 HICUT

*/

// 
// pro Bit gibt es aus "Einfachheitsgründen" ein Byte 96
unsigned char daten[104];
//

unsigned char segmente[7];
// fuer debugausgaben
//char Buffer[7];	

//
// immer nur eine Zahl, also 0 bis 9
void display_convert_number(unsigned char number,unsigned char segmente[6])
{
	//uart_puts("display_convert_number():\r\n");

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

void display_write_meter(uint32_t value)
{
	//uart_puts("display_write_meter():\r\n");

	/*
	Zahlen sind die 76
	Balken
	82 1.
	88 2.
	91 3.
	92 4.
	95 5.
	93 6.
	98 7.
	97 8.
	
	*/
	daten[76]=0x01;

	if(value > 500)
	{
		daten[82]=0x01;
		daten[88]=0x01;
		daten[91]=0x01;
		daten[92]=0x01;
		daten[95]=0x01;
		daten[93]=0x01;
		daten[98]=0x01;
		daten[97]=0x01;
	}
	else if(value > 320)
	{
		daten[82]=0x01;
		daten[88]=0x01;
		daten[91]=0x01;
		daten[92]=0x01;
		daten[95]=0x0;
		daten[93]=0x0;
		daten[98]=0x0;
		daten[97]=0x0;
	}
	else
	{
		daten[82]=0x01;
		daten[88]=0x0;
		daten[91]=0x0;
		daten[92]=0x0;
		daten[95]=0x0;
		daten[93]=0x0;
		daten[98]=0x0;
		daten[97]=0x0;
	}
	// TODO: hier besser umwandlung, vergleichen und nur schreiben wenn notwendig!
	
	display_send();
}

void display_write_modus(unsigned char modus)
{
	//uart_puts("display_write_modus():\r\n");

	// RX 3
	// TX 2
	if(modus == 0)
	{
		daten[3]=0x01;
		daten[2]=0x0;
	}
	else
	{
		daten[3]=0x0;
		daten[2]=0x01;
	}
	display_send();
}

void display_write_mod(unsigned char mod)
{
	//uart_puts("display_write_mod():\r\n");

	// 1 FM 21
	// 2 AM 12
	// 3 USB 39
	// 4 LSB 30
	
	if(mod == 1)
	{
		daten[12]=0x0;
		daten[21]=0x01;
		daten[30]=0x0;
		daten[39]=0x0;
	}
	else if(mod == 2)
	{
		daten[12]=0x01;
		daten[21]=0x0;
		daten[30]=0x0;
		daten[39]=0x0;
	}
	else if(mod == 3)
	{
		daten[12]=0x0;
		daten[21]=0x0;
		daten[30]=0x0;
		daten[39]=0x01;
	}
	else if(mod == 4)
	{
		daten[12]=0x0;
		daten[21]=0x0;
		daten[30]=0x01;
		daten[39]=0x0;
	}
	display_send();
}

void display_write_frequenz(unsigned int frequenz)
{
	//uart_puts("display_write_frequenz():\r\n");

	// Punkt anmachen
	daten[37]=0x01;

	// 
	// zerlegen
	unsigned char x;
	
	x = frequenz / 10000;
	segmente[0]=24;
	segmente[1]=27;
	segmente[2]=29;
	segmente[3]=25;
	segmente[4]=22;
	segmente[5]=23;
	segmente[6]=26;
	display_convert_number(x,segmente);
	
	x = frequenz % 10000 / 1000;
	segmente[0]=33;
	segmente[1]=36;
	segmente[2]=38;
	segmente[3]=34;
	segmente[4]=31;
	segmente[5]=32;
	segmente[6]=35;
	display_convert_number(x,segmente);
	
	x = frequenz % 1000 / 100;
	segmente[0]=42;
	segmente[1]=45;
	segmente[2]=47;
	segmente[3]=43;
	segmente[4]=40;
	segmente[5]=41;
	segmente[6]=44;
	display_convert_number(x,segmente);
	
	x = frequenz % 100 / 10;
	segmente[0]=51;
	segmente[1]=54;
	segmente[2]=56;
	segmente[3]=52;
	segmente[4]=49;
	segmente[5]=50;
	segmente[6]=53;
	display_convert_number(x,segmente);
	
	x = frequenz % 10;
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
	//uart_puts("display_write_channel():\r\n");

	// 
	// zerlegen
	unsigned char x;
	
	x = channel / 10;
	segmente[0]=69;
	segmente[1]=72;
	segmente[2]=74;
	segmente[3]=70;
	segmente[4]=68;
	segmente[5]=66;
	segmente[6]=71;
	display_convert_number(x,segmente);
	if(channel == 0)
	{
		daten[segmente[0]]=0x0;
		daten[segmente[1]]=0x0;
		daten[segmente[2]]=0x0;
		daten[segmente[3]]=0x0;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x0;
		daten[segmente[6]]=0x0;
	}
	x = channel % 10;
	segmente[0]=78;
	segmente[1]=81;
	segmente[2]=83;
	segmente[3]=80;
	segmente[4]=77;
	segmente[5]=75;
	segmente[6]=79;
	display_convert_number(x,segmente);
	if(channel == 0)
	{
		daten[segmente[0]]=0x0;
		daten[segmente[1]]=0x0;
		daten[segmente[2]]=0x0;
		daten[segmente[3]]=0x0;
		daten[segmente[4]]=0x0;
		daten[segmente[5]]=0x0;
		daten[segmente[6]]=0x0;
	}
	
	display_send();
}

void display_init(void)
{
	//uart_puts("display_init():\r\n");

	i2c_start_wait(0x70);
	i2c_write(0xe0);     			// Device Select 0
  i2c_write(0xcf);	  	// multiplex 1100 1111 , 3 BP's
  i2c_write(0xF8);   				// 1111 1000 = immer die 1. RAM Bank (macht eh keinen Sinn...)
  i2c_write(0xF0);   				// 1111 0000 = Blinken, alles abgeschaltet 
	//i2c_stop();
}

void display_send(void)
{
	//uart_puts("display_send():\r\n");

	display_init();
	// die Daten die dann wirklich über den i2c Bus gehen
	unsigned char daten2send=0;
	int y,addr;
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
		//i2c_stop();
		y=y+6;
		addr=addr+2;
	}
}
