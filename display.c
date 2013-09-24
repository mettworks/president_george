
#define F_CPU 18432000UL
//#define BAUD 9600UL
#define debug

#include <avr/io.h>
#include <util/delay.h>
//#include <util/setbaud.h> 
//#include <avr/interrupt.h>
#include <i2cmaster.h>
//#include "eeprom.h"


// es werden 99 Bits (3x33 Segmente) benötigt

unsigned char d_byte00=0;
unsigned char d_byte01=0;
unsigned char d_byte02=0;
unsigned char d_byte03=0;
unsigned char d_byte04=0;
unsigned char d_byte05=0;
unsigned char d_byte06=0;
unsigned char d_byte07=0;
unsigned char d_byte08=0;
unsigned char d_byte09=0;
unsigned char d_byte10=0;
unsigned char d_byte11=0;
unsigned char d_byte12=0;

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




display_init()
{
  i2c_start_wait(0x70);    // Adresse, alle Bits auf 0 UND das R/W Bit!
  i2c_write(0xe0);  // ??   // IMHO Device Select 0
  i2c_write(0xcf);	  // multiplex 1100 1111
  i2c_write(0xFB);   // Bank select 2	    11111011 
  i2c_write(0xF0);   // Blink select (0xF0= off/0xF2= on) 
  /*
	i2c_write(0);
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x);
	i2c_write(0xff);
	i2c_write(0xff);
	
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0xff);
	
	i2c_write(0x0);
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0x0);
	i2c_write(0xff);
	
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0xff);
	i2c_write(0xff);
	*/
	i2c_stop();
}

display_send()
{
	i2c_start_wait(0x70);

	// Adresse setzen
	i2c_write(0);
	
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x0);
	
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x1);
	i2c_write(0x0);
	i2c_write(0x0);
	
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x0);
	i2c_write(0x0);
	
	i2c_write(0x00);
	i2c_write(0x00);
	i2c_write(0x00);
	i2c_write(0x00);
	i2c_write(0x00);
	
	i2c_stop();
}