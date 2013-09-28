
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
// 		0 1 2 3 4 5 6 ...
//	0 1 4 7 1 4 7
//	1 2 5 8 2 5 8
//	2	3 6 ! 3 6 !
//	3

unsigned char daten[13];

display_write_channel(unsigned char channel)
{
	daten[0]=0xff;
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
	i2c_start_wait(0x70);
	itoa(daten[0], Buffer, 10 );

	// Speicher Adresse setzen
	i2c_write(0);
	uart_puts("1. Byte: ");
	uart_puts(Buffer);
	uart_puts("\r\n");
	i2c_write(daten[0]);
	i2c_stop();
}
