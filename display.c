
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
// 0 | 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 UNBENUTZT
// 1 | 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 UNBENUTZT
// 2 | 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! UNBENUTZT
// 3 |
//
// 2. Lauf Übertragung in den RAM des Display Controllers:
//
// Startadresse 6
//
//   | 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 29
// -----------------------------------------------------------------------------------------------------------------
// 0 |     1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 1 4 7 UNBENUTZT
// 1 |     2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 2 5 8 UNBENUTZT
// 2 |     3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! 3 6 ! UNBENUTZT
// 3 |


// Ablauf (4 BP's)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 		| 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
//	-----------------------------------------------------------------------------------------------------------------
//	0 | 1 5 1 5 1 5 1 5 1 5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5
//  1 | 2 6 2 6 2 6 2 6 2 6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6
//  2 | 3 7 2 6 2 6 2 6 2 6  3  7  2  6  2  6  2  6  2  6  3  7  2  6  2  6  2  6  2  6  3  7  2  6  2  6  2  6  2  6
//  3 | 4 8 4 8 4 8 4 8 4 8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  <-- UNBENUTZT, wird immer auf 0 gesetzt!
// 
// pro Bit gibt es aus "Einfachheitsgründen" ein Byte
unsigned char daten[103];
//
// die Daten die dann wirklich über den i2c Bus gehen
unsigned char daten2send;

display_write_channel(unsigned char channel)
{
	daten[0]=0x00;
	daten[1]=0x00;
	daten[2]=0x01;
	daten[3]=0x00;
	daten[4]=0x00;
	daten[5]=0x00;
	daten[6]=0x00;
	daten[7]=0x00;
	daten[8]=0x00;
	daten[9]=0x00;
	daten[10]=0x00;
	daten[11]=0x00;
	daten[12]=0x00;	
	daten[13]=0x00;
	daten[14]=0x00;
	daten[15]=0x00;
	daten[16]=0x00;
	daten[17]=0x00;
	daten[18]=0x00;
	daten[19]=0x00;
	daten[20]=0x00;
	daten[21]=0x00;
	daten[22]=0x00;
	daten[23]=0x00;
	daten[24]=0x00;	
	daten[25]=0x00;
	daten[26]=0x00;
	daten[27]=0x00;
	daten[28]=0x00;
	daten[29]=0x00;
	daten[30]=0x00;
	daten[31]=0x00;
	daten[32]=0x00;
	daten[33]=0x00;
	daten[34]=0x00;
	daten[35]=0x00;
	daten[36]=0x00;
	daten[37]=0x00;
	daten[38]=0x00;
	daten[39]=0x00;
	daten[40]=0x00;
	daten[41]=0x00;
	daten[42]=0x00;
	daten[43]=0x00;
	daten[44]=0x00;
	daten[45]=0x00;
	daten[46]=0x00;	
	daten[47]=0x00;
	daten[48]=0x00;
	daten[49]=0x00;
	daten[50]=0x00;
	daten[51]=0x00;
	daten[52]=0x00;
	daten[53]=0x00;
	daten[54]=0x00;
	daten[55]=0x00;
	daten[56]=0x00;
	daten[57]=0x00;	
	daten[58]=0x00;
	daten[59]=0x00;
	daten[60]=0x00;
	daten[61]=0x00;
	daten[62]=0x00;
	daten[63]=0x00;
	daten[64]=0x00;
	daten[65]=0x00;
	daten[66]=0x00;
	daten[67]=0x00;
	daten[68]=0x00;
	daten[69]=0x00;
	daten[70]=0x00;
	daten[71]=0x00;
	daten[72]=0x00;
	daten[73]=0x00;
	daten[74]=0x00;
	daten[75]=0x00;
	daten[76]=0x00;
	daten[77]=0x00;
	daten[78]=0x00;
	daten[79]=0x00;	
	daten[80]=0x00;
	daten[81]=0x00;
	daten[82]=0x00;
	daten[83]=0x00;
	daten[84]=0x00;
	daten[85]=0x00;
	daten[86]=0x00;
	daten[87]=0x00;
	daten[88]=0x00;
	daten[89]=0x00;
	daten[90]=0x00;
	daten[91]=0x00;	
	daten[92]=0x00;
	daten[93]=0x00;
	daten[94]=0x00;
	daten[95]=0x00;
	daten[96]=0x00;
	daten[97]=0x00;
	daten[98]=0x00;
	daten[99]=0x00;
	// ab hier nur zum füllen...
	daten[100]=0x00;
	daten[101]=0x00;
	daten[102]=0x00;
	daten[103]=0x00;	
	display_send();
}

display_send()
{
	uart_puts("Beginn display_send():");
	uart_puts("\r\n");
  char Buffer[7];	
	char bits[7];
	int y,addr;
	i2c_start_wait(0x70);
	i2c_write(0xe0);     			// Device Select 0
  i2c_write(0xcf);	  	// multiplex 1100 1111 , 3 BP's
	//i2c_write(0xcc);					// 1100 1100 = kein Power Saving, Display enabled, 1/3 bias, 4 BP's
  i2c_write(0xF8);   				// 1111 1000 = immer die 1. RAM Bank (macht eh keinen Sinn...)
	//i2c_write(0xfb);					// test 2. Bank
  i2c_write(0xF0);   				// 1111 0000 = Blinken, alles abgeschaltet 
	i2c_stop();
	
	y=0;
	addr=0;

	for ( y = 0; y < 99; )
	{
		if(daten[y] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x00);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x00);
		}
		
		if(daten[y+1] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x01);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x01);
		}
		if(daten[y+2] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x02);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x02);
		}
		
		if(daten[y+3] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x03);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x03);
		}

		if(daten[y+4] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x04);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x04);
		}
		if(daten[y+5] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x05);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x05);
		}				
		if(daten[y+6] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x06);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x06);
		}				
		if(daten[y+7] & (1<<0x00)) 
		{
			uart_puts("1");
			daten2send |= (1<<0x07);
		}
		else
		{
			uart_puts("0");
			daten2send &= ~(1<<0x07);
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
		y=y+5;
		addr=addr+2;
	}
}
