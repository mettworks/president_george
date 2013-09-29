
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
// Befuellung des Speichers mit 3 Backplanes
//
// 1 Byte wird an den Speicher übertragen, das wird auf 3 Bänke verteilt, dabei fehlt immer ein Bit!
//
// Ablauf (3 BP's)
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


//
//

//
// 
// Ablauf (4 BP's)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Speicher für interne Verarbeitung
// Array daten, es werden immer nur 3 Bits verwendet
//
//  	| 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 UNBENUTZT
//	-----------------------------------------------------------------------------------------------------------------
//  0 | 1 1
//  1 | 2 2 USW.
//  2 | 3 3
//  3 |
//
//
// 		| 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
//	-----------------------------------------------------------------------------------------------------------------
//	0 | 1 5 1 5 1 5 1 5 1 5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5  1  5
//  1 | 2 6 2 6 2 6 2 6 2 6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6  2  6
//  2 | 3 7 2 6 2 6 2 6 2 6  3  7  2  6  2  6  2  6  2  6  3  7  2  6  2  6  2  6  2  6  3  7  2  6  2  6  2  6  2  6
//  3 | 4 8 4 8 4 8 4 8 4 8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  4  8  <-- UNBENUTZT, wird immer auf 0 gesetzt!
 
// in ein Feld von Daten kommen 3 Bits
unsigned char daten[33];

unsigned char daten2send;

display_write_channel(unsigned char channel)
{
	daten[0]=0x00;
	daten[1]=0x00;
	daten[2]=0x00;
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
	display_send();
	
	daten[0]=0x01;
	daten[1]=0x01;
	daten[2]=0x01;
	daten[3]=0x01;
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
	display_send();
	
	
}

/*
display_init()
{

	uart_puts("display_init()\r\n");
	uart_puts("\r\n");

  i2c_start_wait(0x70);    	// Adresse, alle Bits auf 0 UND das R/W Bit!
	//
	// Funktion 1. Bit Kommandos:
	// 0 letztes Kommando, danach werden Daten erwartet
	// 1 noch mehr Kommandos
  i2c_write(0xe0);     			// Device Select 0
  i2c_write(0xcf);	  	// multiplex 1100 1111 , 3 BP's
	//i2c_write(0xcc);					// 1100 1100 = kein Power Saving, Display enabled, 1/3 bias, 4 BP's
  //i2c_write(0xF8);   				// 1111 1000 = immer die 1. RAM Bank (macht eh keinen Sinn...)
	i2c_write(0xfb);					// test 2. Bank
  i2c_write(0xF0);   				// 1111 0000 = Blinken, alles abgeschaltet 
	//i2c_write(0);
	//i2c_write(0x01);
	
	i2c_stop();
	//while(1)
	//{
	//}
	daten[0]=0x0;
	daten[1]=0x0;
	daten[2]=0x0;
	daten[3]=0x0;
	daten[4]=0x0;
	daten[5]=0x0;
	daten[6]=0x0;
	daten[7]=0x0;
	daten[8]=0x0;
	daten[9]=0x0;
	daten[10]=0x0;
	daten[11]=0x0;
	daten[12]=0x0;	
	daten[13]=0x0;
	daten[14]=0x0;
	daten[15]=0x0;
	daten[16]=0x0;
	daten[17]=0x0;
	daten[18]=0x0;
	daten[19]=0x0;
	daten[20]=0x0;
	daten[21]=0x0;
	daten[22]=0x0;
	daten[23]=0x0;
	daten[24]=0x0;	
	daten[25]=0x0;
	daten[26]=0x0;
	daten[27]=0x0;
	daten[28]=0x0;
	daten[29]=0x0;
	daten[30]=0x0;
	daten[31]=0x0;
	daten[32]=0x0;
	daten[33]=0x0;
	//display_send();
	//while(1)
	//{
	//}
}
*/
display_send()
{
  char Buffer[7];	
	char bits[7];
	int y;
	i2c_start_wait(0x70);
	i2c_write(0xe0);     			// Device Select 0
  i2c_write(0xcf);	  	// multiplex 1100 1111 , 3 BP's
	//i2c_write(0xcc);					// 1100 1100 = kein Power Saving, Display enabled, 1/3 bias, 4 BP's
  //i2c_write(0xF8);   				// 1111 1000 = immer die 1. RAM Bank (macht eh keinen Sinn...)
	i2c_write(0xfb);					// test 2. Bank
  i2c_write(0xF0);   				// 1111 0000 = Blinken, alles abgeschaltet 
	/*
	uart_puts("Ausgabe Sendedaten");
	uart_puts("\r\n");
	uart_puts("######################");
	uart_puts("\r\n\r\n");
	
	uart_puts("\r\n");
	*/
	i2c_write(0x00);
	y=0;
	for ( y = 0; y < 33; )
	{
		/*
		itoa(daten[y], Buffer, 10);
		uart_puts(" Byte0: ");
		uart_puts(Buffer);
		uart_puts(" ");
		itoa(daten[y+1], Buffer, 10);
		uart_puts(" Byte1: ");
		uart_puts(Buffer);
		uart_puts("\r\n");
		*/
		if(daten[y] & (1<<0x00)) 
		{
			//uart_puts("Bit 1 ist 1");
			daten2send |= (1<<0x00);
		}
		else
		{
			//uart_puts("Bit 1 ist 0");
			daten2send &= ~(1<<0x00);
		}
		if(daten[y] & (1<<0x01)) 
		{
			//uart_puts("Bit 2 ist 1");
			daten2send |= (1<<0x01);
		}
		else
		{
			//uart_puts("Bit 2 ist 0");
			daten2send &= ~(1<<0x01);
		}
		if(daten[y] & (1<<0x02)) 
		{
			daten2send |= (1<<0x02);
		}
		else
		{
			daten2send &= ~(1<<0x02);
		}
		if(daten[y+1] & (1<<0x00)) 
		{
			daten2send |= (1<<0x04);
		}
		else
		{
			daten2send &= ~(1<<0x04);
		}

		if(daten[y+1] & (1<<0x01)) 
		{
			daten2send |= (1<<0x05);
		}
		else
		{
			daten2send &= ~(1<<0x05);
		}
		if(daten[y+1] & (1<<0x02)) 
		{
			daten2send |= (1<<0x06);
		}
		else
		{
			daten2send &= ~(1<<0x06);
		}				
		
		y=y+2;
		i2c_write(daten2send);
		/*
		itoa(daten2send, Buffer, 10);
		uart_puts(" daten2send: ");
		uart_puts(Buffer);
		uart_puts("\r\n");
		*/
	}
	i2c_stop();

	
	//uart_puts("\r\n\r\n");
}
