/*
avrdude -p atmega128 -P /dev/ttyACM0 -c stk500v2 -v -Uefuse:w:0xFF:m -U hfuse:w:0xC9:m -U lfuse:w:0xDF:m
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "memory.h"
#include "display.h"
#include "led.h"
#include "transceiver.h"
#include "operating.h"
#include "i2c.h"
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

int ichbinaus=0;
int led_farbe=0;
unsigned int led_dimm1=20;
unsigned int led_dimm2=20;
unsigned char memory[6];
int mod = 1;
unsigned int freq = 27205;
unsigned int step = 5;

//
// IRQ für Spannungsabfall
// wegspeichern der Einstellungen im EEPROM
// beim wiederkommen von VCC, wird durch ein RC Glied Reset ausgelöst
ISR (INT4_vect)
{
	#ifdef debug
	uart_puts("INT4\r\n");
	#endif
	save2memory();
	while(1)
	{
		_delay_ms(1000);
		#ifdef debug
		uart_puts("1 Sekunde\r\n");
		#endif
	}
}

ISR (INT5_vect)
{
	if(ichbinaus == 1)
	{
		#ifdef debug
		uart_puts("AN\r\n");
		#endif
	  PORTA |= (1<<PA7);	// einschalten
		led_pwm(1,0);
		led_pwm(2,0);
		led_pwm(3,0);
		led_pwm(4,0);
		led_pwm(5,0);
		led_pwm(6,0);
		led_pwm(7,0);
		led_pwm(8,0);
		led_pwm(9,0);
		led_pwm(10,0);
		led_pwm(11,0);
		led_pwm(12,0);
		init_geraet();
		ichbinaus=0;
	}
	else
	{
		ichbinaus=1;
		#ifdef debug
		uart_puts("AUS\r\n");
		#endif
		save2memory();
		PORTA &= ~(1<<PA7);	// ausschalten...
		led_pwm(1,0);
		led_pwm(2,0);
		led_pwm(3,0);
		led_pwm(4,0);
		led_pwm(5,0);
		led_pwm(6,0);
		led_pwm(7,0);
		led_pwm(8,0);
		led_pwm(9,0);
		led_pwm(10,0);
		led_pwm(11,0);
		led_pwm(12,0);
	}
}
/*
ISR (INT6_vect)
{
	#ifdef debug
	uart_puts("INT6\r\n");
	#endif
	keycheck();
}
*/
ISR (INT7_vect)
{
	/*
	#ifdef debug
	uart_puts("INT7\r\n");
	#endif
	*/
	keycheck();
}

ISR (TIMER0_OVF_vect)
{
	#ifdef debug
	uart_puts("INT Timer0\r\n");
	#endif
	keycheck();
}

ISR(BADISR_vect)
{
	#ifdef debug
  uart_puts("RESET AUSGELOEST!\r\n");
	uart_puts("nicht definierter Interrupt!\r\n");
	uart_puts("STOP\r\n");
	#endif
	while(1)
	{
	}
}

//
// sehr unelegant, muss mit einem Timer gemacht werden
// erstmal geht es nur um den HW Test ... ^^
void scan(void)
{
	while(1)
	{
		freq=freq+step;
		tune(freq,step);
		_delay_ms(250);
		if ( !(PINA & (1<<PINA2)) )
		{
			#ifdef debug
			uart_puts("Treffer\r\n");
			#endif
			break;
		}
	}
}

ISR(ADC_vect)
{
	uint16_t x;
	//x += (ADCH<<8);
	// oder besser
	x = ADCW;
	#ifdef debug
	char s[7];
	uart_puts("Messwert: ");
	uart_puts( itoa( x, s, 10 ) );
	uart_puts("\r\n");
	#endif
}

int main(void) 
{
	cli();
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
	uart_puts("Beginn main()\r\n");
	#endif

  //
  // Ein und Ausgaenge
	// PE4, INT4 ist VCC Kontrolle					-> Eingang
	// PE7, INT7 ist 1. Port Expander				-> Eingang
	// PE6, INT6 ist 2. Port Expander				-> Eingang
 	// PE5, INT5 ist Taster 1, Ein/Aus			-> Eingang
  // PA4 ist Latch PLL      							-> Ausgang		PIN10 vom Mainboard     
  // PA6 ist Latch Treiber  							-> Ausgang		PIN7 vom Mainboard     
  // PA5 ist Data	    										-> Ausgang		PIN9 vom Mainboard   
  // PA3 ist Clock	    									-> Ausgang		PIN8 vom Mainboard     
	// PC0 ist LED Rot											-> Ausgang
	// PC1 ist LED Grün 										-> Ausgang
	// PA7 ist Ein/Aus											-> Ausgang
	// PA2 ist "Busy" (Rauschsperre offen)	-> Eingang
	
	// PE4
	DDRE &= ~(1<<PE4);	// Eingang
	// PE7
	DDRE &= ~(1<<PE7);	// Eingang
  PORTE |= (1<<PE7);	// internen Pullup aktivieren
	// PE6
	DDRE &= ~(1<<PE6);	// Eingang
  PORTE |= (1<<PE6);	// internen Pullup aktivieren
	// PE5
  DDRE &= ~(1<<PE5);	// Eingang
  PORTE |= (1<<PE5);	// internen Pullup aktivieren

	// PC0
	DDRC |= (1<<PC0);
	// PC1
	DDRC |= (1<<PC1);
	// PA7
	DDRA |= (1<<PA7);
  PORTA |= (1<<PA7);	// einschalten
	// PA2
	DDRA &= ~(1<<PA2);	// Eingang
	
	//
	// Interrupts
	// INT4 wird bei fallender Flanke ausgelöst -> VCC weg
	// INT7 wird für den 1. i2c Port Expander genutzt
	// (warum nicht bei fallender Flanke? Hmmm!)
	
	EICRB |= (0 << ISC70) | (0 << ISC71);    // 0 löst aus
	EICRB |= (0 << ISC60) | (0 << ISC61);    // 0 löst aus
  EICRB |= (0 << ISC40) | (1 << ISC41);    // fallende Flanke
	EICRB |= (0 << ISC50) | (1 << ISC51);    // fallende Flanke
	EIMSK |= (1 << INT4) | (1<< INT7) | (1<< INT5) | (1<< INT6);
	
	// TODO, hier muss noch ein besserer Vorteiler gesucht werden... Je nachdem wie schnell die Tasten sind...
  // Timer 0 konfigurieren
  TCCR0 = (1<<CS01); // Prescaler 8
	//TCCR0|=(1<<CS00) | (1<<CS01);
	
/*
	// EEPROM
	unsigned char IOReg;
	DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   	//SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
	IOReg   = SPSR;                            		//SPI Status und SPI Datenregister einmal auslesen
	IOReg   = SPDR;
	PORTB |= (1<<PB0);                         	//ChipSelect aus
	
	unsigned char H_Add=0b00000000;    						// Adresse
  unsigned char M_Add=0b00000000;
	unsigned char L_Add=0b00000000;
	memory[0] = ByteReadSPI(H_Add,L_Add,M_Add);  //Byte an Test Adresse auslesen und der Variablen out übergeben
	H_Add=0b00000000;
  M_Add=0b00000000;
	L_Add=0b00000001;
	memory[1] = ByteReadSPI(H_Add,L_Add,M_Add);  //Byte an Test Adresse auslesen und der Variablen out übergeben

	freq = memory[1] + (memory[0] << 8);

	if(freq == 0)
	{
		#ifdef debug
		uart_puts("ICH HABE ALZHEIMER!\r\n");
		#endif
		freq=27000;
	}
*/
	mod=1;
	i2c_init();
	init_geraet();
	display_init();
	init_led();
	led_helligkeit1(led_dimm1);
	led_helligkeit2(led_dimm2);
	led_color(led_farbe);

	display_write_modus(0);
	sei();
	while(1)
	{

	}

/*

	mod=1;
	init_geraet();	
	
	//
	// Gemessen wird an ADC1, es kann "Gain" (10x) genutzt werden, dazu liegt ADC0 auf GND
	// dann muss ADMUX |= (1 << MUX3) | (1<<MUX0); gesetzt sein

  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Set ADC prescaler to 128 - 125KHz sample rate @ 16MHz

  ADMUX |= (1 << REFS0) | (1 << REFS1); 	// 2.65 V

	
	// ADC1 auswählen
	ADMUX |= (1<<MUX0);
	//ADMUX |= (1 << MUX3) | (1<<MUX0);
	ADCSRA |= (1 << ADFR);  // Set ADC to Free-Running Mode
  ADCSRA |= (1 << ADEN);  // Enable ADC
	ADCSRA |= (1 << ADIE);  // Enable ADC Interrupt
  sei();   // Enable Global Interrupts
	ADCSRA |= (1 << ADSC);  // Start A2D Conversions

	for(;;)  // Loop Forever
  {
  }
	*/
} 
