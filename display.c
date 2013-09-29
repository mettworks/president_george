
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
// Befuellung des Speichers mit 3 Backplanes
//
// 1 Byte wird an den Speicher übertragen, das wird auf 3 Bänke verteilt, dabei fehlt immer ein Bit!
//
// Ablauf
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 1. Lauf Übertragung in den RAM des Display Controllers:
//
// Startadresse 0
//
// 		| 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 29
//	-----------------------------------------------------------------------------------------------------------------
//	0 | 1 4 7 1 4 7 1 4 7 1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  UNBENUTZT
//	1 | 2 5 8 2 5 8 2 5 8 2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  UNBENUTZT
//	2	| 3 6 ! 3 6 ! 3 6 ! 3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  UNBENUTZT
//	3 |
//
// 2. Lauf Übertragung in den RAM des Display Controllers:
//
// Startadresse 6
//
// 		| 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 29
//	-----------------------------------------------------------------------------------------------------------------
//	0 |     1 4 7 1 4 7 1 4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  1  4  7  UNBENUTZT
//	1 |     2 5 8 2 5 8 2 5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  2  5  8  UNBENUTZT
//	2	|     3 6 ! 3 6 ! 3 6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  3  6  !  UNBENUTZT
//	3 |


// in ein Feld von Daten kommen 3 Bits
unsigned char daten[33];

unsigned char daten2send;

display_write_channel(unsigned char channel)
{
	daten[0]=0xff;
	daten[1]=0xff;
	daten[2]=0xff;
	daten[3]=0xff;
	daten[4]=0xff;
	daten[5]=0xff;
	daten[6]=0xff;
	daten[7]=0xff;
	daten[8]=0xff;
	daten[9]=0xff;
	daten[10]=0xff;
	daten[11]=0xff;
	daten[12]=0xff;	
	daten[13]=0xff;
	daten[14]=0xff;
	daten[15]=0xff;
	daten[16]=0xff;
	daten[17]=0xff;
	daten[18]=0xff;
	daten[19]=0xff;
	daten[20]=0xff;
	daten[21]=0xff;
	daten[22]=0xff;
	daten[23]=0xff;
	daten[24]=0xff;	
	daten[25]=0xff;
	daten[26]=0xff;
	daten[27]=0xff;
	daten[28]=0xff;
	daten[29]=0xff;
	daten[30]=0xff;
	daten[31]=0xff;
	daten[32]=0xff;
	daten[33]=0xff;

	display_send();
}


display_init()
{
	uart_puts("display_init()\r\n");
	uart_puts("\r\n");
  i2c_start_wait(0x70);    // Adresse, alle Bits auf 0 UND das R/W Bit!
  i2c_write(0xe0);  // ??   // IMHO Device Select 0
  i2c_write(0xcf);	  // multiplex 1100 1111
  i2c_write(0xFB);   // Bank select 2	    11111011 
  i2c_write(0xF0);   // Blink select (0xF0= off/0xF2= on) 
	i2c_stop();
}

display_send()
{
  char Buffer[7];	
	char bits[7];
	int y;
	i2c_start_wait(0x70);

	uart_puts("Ausgabe Sendedaten");
	uart_puts("\r\n");
	uart_puts("######################");
	uart_puts("\r\n\r\n");
	
	uart_puts("Startadresse 0");
	uart_puts("\r\n");
	i2c_write(0);
	y=0;
	for ( y = 0; y < 33; )
	{
		bits[0] = (daten[y] & (1 << 0)) >> 0;
		bits[1] = (daten[y] & (1 << 1)) >> 1;
		bits[2] = (daten[y] & (1 << 2)) >> 2;
		
		bits[3] = (daten[y+1] & (1 << 1)) >> 1;
		bits[4] = (daten[y+1] & (1 << 2)) >> 2;
		bits[5] = (daten[y+1] & (1 << 3)) >> 3;
		
		bits[6] = (daten[y+2] & (1 << 1)) >> 1;
		bits[7] = (daten[y+2] & (1 << 2)) >> 2;

		if(bits[0] == 1)
		{
			daten2send = daten2send | (1 << 0);
		}
		else
		{
			daten2send = daten2send &~ (1 << 0);
		}
		
		if(bits[1] == 1)
		{
			daten2send = daten2send | (1 << 1);
		}
		else
		{
			daten2send = daten2send &~ (1 << 1);
		}
		if(bits[2] == 1)
		{
			daten2send = daten2send | (1 << 2);
		}
		else
		{
			daten2send = daten2send &~ (1 << 2);
		}
		if(bits[3] == 1)
		{
			daten2send = daten2send | (1 << 3);
		}
		else
		{
			daten2send = daten2send &~ (1 << 3);
		}
		if(bits[4] == 1)
		{
			daten2send = daten2send | (1 << 4);
		}
		else
		{
			daten2send = daten2send &~ (1 << 4);
		}
		
		if(bits[5] == 1)
		{
			daten2send = daten2send | (1 << 5);
		}
		else
		{
			daten2send = daten2send &~ (1 << 5);
		}
		if(bits[6] == 1)
		{
			daten2send = daten2send | (1 << 6);
		}
		else
		{
			daten2send = daten2send &~ (1 << 6);
		}
		if(bits[7] == 1)
		{
			daten2send = daten2send | (1 << 7);
		}
		else
		{
			daten2send = daten2send &~ (1 << 7);
		}
		
		y=y+3;
		i2c_write(daten2send);
		itoa(daten2send, Buffer, 10);
		uart_puts(" daten2send: ");
		uart_puts(Buffer);
		uart_puts("\r\n");
	}
	i2c_stop();
	
	uart_puts("Startadresse 2");
	uart_puts("\r\n");
	i2c_start_wait(0x70);
	i2c_write(2);
	y=2;
	for ( y = 2; y < 35; )
	{
		bits[0] = (daten[y] & (1 << 0)) >> 0;
		bits[1] = (daten[y] & (1 << 1)) >> 1;
		bits[2] = (daten[y] & (1 << 2)) >> 2;
		
		bits[3] = (daten[y+1] & (1 << 1)) >> 1;
		bits[4] = (daten[y+1] & (1 << 2)) >> 2;
		bits[5] = (daten[y+1] & (1 << 3)) >> 3;
		
		bits[6] = (daten[y+2] & (1 << 1)) >> 1;
		bits[7] = (daten[y+2] & (1 << 2)) >> 2;

		if(bits[0] == 1)
		{
			daten2send = daten2send | (1 << 0);
		}
		else
		{
			daten2send = daten2send &~ (1 << 0);
		}
		
		if(bits[1] == 1)
		{
			daten2send = daten2send | (1 << 1);
		}
		else
		{
			daten2send = daten2send &~ (1 << 1);
		}
		if(bits[2] == 1)
		{
			daten2send = daten2send | (1 << 2);
		}
		else
		{
			daten2send = daten2send &~ (1 << 2);
		}
		if(bits[3] == 1)
		{
			daten2send = daten2send | (1 << 3);
		}
		else
		{
			daten2send = daten2send &~ (1 << 3);
		}
		if(bits[4] == 1)
		{
			daten2send = daten2send | (1 << 4);
		}
		else
		{
			daten2send = daten2send &~ (1 << 4);
		}
		
		if(bits[5] == 1)
		{
			daten2send = daten2send | (1 << 5);
		}
		else
		{
			daten2send = daten2send &~ (1 << 5);
		}
		if(bits[6] == 1)
		{
			daten2send = daten2send | (1 << 6);
		}
		else
		{
			daten2send = daten2send &~ (1 << 6);
		}
		if(bits[7] == 1)
		{
			daten2send = daten2send | (1 << 7);
		}
		else
		{
			daten2send = daten2send &~ (1 << 7);
		}
		
		y=y+3;
		i2c_write(daten2send);
		itoa(daten2send, Buffer, 10);
		uart_puts(" daten2send: ");
		uart_puts(Buffer);
		uart_puts("\r\n");
	}
	i2c_stop();
	
	uart_puts("\r\n\r\n");
}
