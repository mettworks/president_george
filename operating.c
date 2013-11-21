#include <avr/interrupt.h>
#include "i2c.h"
#include "led.h"
#include "transceiver.h"
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

unsigned long keys;
extern int led_farbe;
extern unsigned int led_dimm1;
extern unsigned int led_dimm2;
extern unsigned int step;
extern unsigned int freq;

// 2 Byte zurück
unsigned short keysauslesendirekt(unsigned char destaddr)
{
	unsigned char byte0;
	unsigned char byte1;
	unsigned short alles;
	i2c_start_wait(destaddr);
	i2c_write(0x0);
	i2c_rep_start(destaddr + 1);
	byte0=i2c_readAck();
	byte1=i2c_readAck();
	alles = byte1 + (byte0 << 8);
	return alles;
}

unsigned long keysauslesen(void)
{
	// 
	// es werden gleich wieder Interrupts aktiviert, weil:
	// wenn VCC wegfällt, würde auch die i2c Kommunikation wegbrechen, da die Gegenstellen keine Spannung mehr haben
	// so ist sichergestellt, das wir bei einer hängenden i2c Kommunikation auch den INT4 gefasst bekommen
	// ABER ERST WENN I2C FERTIG IST!
	unsigned short blubb1;
	unsigned short blubb2;
	i2c_init();
	blubb1=keysauslesendirekt(0x40);
	blubb2=keysauslesendirekt(0x42);
	// TODO: Wenn hier der i2c Transfer gestoppt wird hängt sich das Teil beim drücken der runter Taste am Mike auf?? 
	//i2c_stop();
	// INT's abschalten
	cli();
	keys=(uint32_t)blubb2 + ((uint32_t)blubb1 << 16);
	return keys;
}

void keycheck(void)
{
	keys=keysauslesen();
	#ifdef debug
	char string[20];
	uart_puts("Daten: ");
	sprintf(string,"%lX",keys);
	uart_puts(string);
	uart_puts("\r\n");
	#endif
	
	cli();
	// 10. Bit
	// TX Anfang, PTT Taste ist gedrückt
	if((keys & 0x100) == 0)
	{
		#ifdef debug
		uart_puts("SW13\r\n");
    uart_puts("Dimmer\r\n");
		#endif
		if(led_dimm1 == 255)
		{
			led_dimm1=128;
		}
		else if(led_dimm1 == 128)
		{
			led_dimm1=64;
		}
		else
		{
			led_dimm1=255;
		}
		led_helligkeit1(led_dimm1);
		
	}
	
	/*
	// + Taste am Mikrofon
	// 31. Bit
	else if((keys & 0x40000000) == 0)
	{
		freq=freq+step;
    tune(freq,step);
	}
	// - Taste am Mikrofon
	// 30. Bit
	else if((keys & 0x20000000) == 0)
	{
		freq=freq-step;
    tune(freq,step);
	}
	*/
	
	
	// 
	// M1
	else if((keys & 0x4000000) == 0)
	{
		#ifdef debug
		uart_puts("M1\r\n");
		#endif
		modulation(1);
	}	
	// 
	// M2
	else if((keys & 0x2000000) == 0)
	{
		#ifdef debug
		uart_puts("M2\r\n");
		#endif
		modulation(2);
	}	
	// 
	// M3
	else if((keys & 0x1000000) == 0)
	{
		#ifdef debug
		uart_puts("M3\r\n");
		#endif
		modulation(3);
	}	
	// 
	// M4
	else if((keys & 0x10000) == 0)
	{
		#ifdef debug
		uart_puts("M4\r\n");
		#endif
		modulation(4);
	}	
	
	// 
	// Drehschalter +
	else if((keys & 0x2) == 0)
	{
		#ifdef debug
		uart_puts("Drehschalter +\r\n");
		#endif
		freq=freq+step;
    tune(freq,step);
	}	
	// 
	// Drehschalter -
	else if((keys & 0x4) == 0)
	{
		#ifdef debug
		uart_puts("Drehschalter -\r\n");
		#endif
		freq=freq-step;
    tune(freq,step);
	}	
	
	/*
	// AM ENDE LASSEN!
	// TX Ende, PTT Taste ist losgelassen
	else if(keys & 0x200)
	{

		#ifdef debug
		uart_puts("SW13\r\n");
    uart_puts("Dimmer\r\n");
		#endif
    rx();
	}	
*/
	//_delay_ms(250);
	if(keys != 0xffffffff)
	{
		// Taste/Tasten sind immer noch gedrückt, also nochmal... :-)
		TIMSK |= (1<<TOIE0);
	}
	else
	{
		// keine Taste/Tasten mehr gedrückt, Timer stoppen
		TIMSK &= ~(1<<TOIE0);
	}
	sei();
}