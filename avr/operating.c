#include <avr/interrupt.h>
#include <util/delay.h>
#include "i2c.h"
#include "led.h"
#include "transceiver.h"
#include "display.h"
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
extern int mod;
extern int cb_mod;
extern int txstat;
extern int modus;
extern unsigned int memory[MEM_SIZE];
extern int cb_channel;
extern unsigned int ctcss;
extern unsigned int rpt;
extern unsigned int echo_ham;
extern unsigned int beep_ham;

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
	//cli();
	keys=(uint32_t)blubb2 + ((uint32_t)blubb1 << 16);
	return keys;
}

void setmodus(int data)
{
	#ifdef debug
	uart_puts("setmodus():\r\n");
	#endif
	memory[1]=data;
	if(data==1)
	{
		#ifdef debug
		uart_puts("HAM Modus\r\n");
		#endif
		tune(freq,step);
		modulation(mod);
		display_write_channel(0);
		modus=1;
		memory[1]=1;
	}
	else
	{
		#ifdef debug
		uart_puts("CB Modus\r\n");
		#endif
		channel(cb_channel);
		modulation(cb_mod);
		modus=2;
		memory[1]=2;
	}
}

void keycheck(void)
{
	//cli();
	keys=keysauslesen();
	
	#ifdef debug
	char string[20];
	uart_puts("Daten: ");
	sprintf(string,"%lX",keys);
	uart_puts(string);
	uart_puts("\r\n");
	#endif
	
	//cli();
	_delay_ms(200);
	//sei();
	// 
	if((keys & 0x20000) == 0)
	{
		#ifdef debug
		uart_puts("RX->TX\r\n");
		#endif
		if(modus == 2)
		{
			#ifdef debug
			uart_puts("kein Sendebetrieb auf dieser Frequenz...\r\n");
			#endif
		}
		else
		{
			tx();
			//TIMSK |= (1<<OCIE1A);
			display_write_modus(1);
			txstat=1;
		}
	}
	else if((keys & 0x1) == 0)
	{
		#ifdef debug
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
	else if((keys & 0x8) == 0)
	{
		#ifdef debug
		uart_puts("CH19\r\n");
		#endif
		if(led_dimm2 == 255)
		{
			led_dimm2=128;
		}
		else if(led_dimm2 == 128)
		{
			led_dimm2=64;
		}
		else
		{
			led_dimm2=255;
		}
		led_helligkeit2(led_dimm2);
	}
	else if((keys & 0x20000000) == 0)
	{
		#ifdef debug
		uart_puts("DC\r\n");
		#endif
		if(led_farbe == 0)
		{
			led_farbe=1;
		}
		else
		{
			led_farbe=0;
		}
		led_color(led_farbe);
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
	
	else if((keys & 0x80000) == 0)
	{
		#ifdef debug
		uart_puts("ECHO\r\n");
		#endif
		if(echo_ham == 0)
		{
			set_echo(1);
		}
		else
		{
			set_echo(0);
		}
	}
	else if((keys & 0x100000) == 0)
	{
		#ifdef debug
		uart_puts("Beep\r\n");
		#endif
		if(beep_ham == 0)
		{
			set_beep(1);
		}
		else
		{
			set_beep(0);
		}
	}
	else if((keys & 0x100) == 0)
	{
		#ifdef debug
		uart_puts("Meter\r\n");
		#endif
		if(ctcss == 0)
		{
			set_ctcss(1);
		}
		else
		{
			set_ctcss(0);
		}
	}	
	else if((keys & 0x200) == 0)
	{
		#ifdef debug
		uart_puts("DW\r\n");
		#endif
		if(rpt == 0)
		{
			set_rpt(1);
		}
		else
		{
			set_rpt(0);
		}
	}	
	else if((keys & 0x400) == 0)
	{
		#ifdef debug
		uart_puts("SELECT\r\n");
		#endif
		tune(28000,5);
	}
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
	// PA
	else if((keys & 0x8000000) == 0)
	{
		#ifdef debug
		uart_puts("PA\r\n");
		#endif
		if(modus==1)
		{
			setmodus(2);
		}
		else if(modus==2)
		{
			setmodus(1);
		}
	}
	// 
	// Drehschalter + ODER CH+
	else if(((keys & 0x2) == 0) || ((keys & 0x80) == 0))
	{
		#ifdef debug
		uart_puts("Drehschalter + ODER Taster +\r\n");
		#endif
		if(modus==1)
		{
			#ifdef debug
			uart_puts("Modus HAM -> Frequenz: ");
			uart_puts(itoa(freq, string, 10));
			uart_puts(" Step: ");
			uart_puts(itoa(step, string, 10));
			uart_puts("\r\n");
			#endif
			freq=freq+step;
			tune(freq,step);
		}
		else
		{
			#ifdef debug
			uart_puts("Modus CB\r\n");
			#endif
			cb_channel++;
			channel(cb_channel);
		}
	}	
	// 
	// Drehschalter - ODER CH-
	else if(((keys & 0x4) == 0) || ((keys & 0x40) == 0))
	{
		#ifdef debug
		uart_puts("Drehschalter - ODER Taster -\r\n");
		#endif
		if(modus==1)
		{
			freq=freq-step;
			tune(freq,step);
		}
		else
		{
			cb_channel--;
			channel(cb_channel);
		}
	}	
	
	// AM ENDE LASSEN!
	// TX Ende, PTT Taste ist losgelassen
	else if(keys & 0x20000)
	{
		if(txstat==1)
		{
			#ifdef debug
			uart_puts("TX->RX\r\n");
			#endif
			//rogerbeep();
			display_write_modus(0);
			rx();
			//TIMSK &= ~(1<<OCIE1A);
			txstat=0;
		}
	}	
	else
	{
		#ifdef debug
		uart_puts("unbekannter Eingang\r\n");
		#endif
	}

	//_delay_ms(250);
	if(keys != 0xffffffff)
	{
		//cli();
		// Taste/Tasten sind immer noch gedrückt, also nochmal... :-)
		//TIMSK |= (1 << OCIE2);
		set_timer0(1);
		//sei();
	}
	else
	{
		//cli();
		// keine Taste/Tasten mehr gedrückt, Timer stoppen
		//TIMSK &= ~(1 << OCIE2);
		set_timer0(0);
		//sei();
	}
	//sei();
}