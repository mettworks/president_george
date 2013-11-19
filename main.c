/*
avrdude -p atmega128 -P /dev/ttyACM0 -c stk500v2 -v -Uefuse:w:0xFF:m -U hfuse:w:0xC9:m -U lfuse:w:0xDF:m
*/

#define F_CPU 18432000UL
#define BAUD 115800UL
//#define BAUD 150UL
//#define debug


unsigned char memory[6];
int mod = 1;
unsigned int freq = 27205;
unsigned int step = 5;

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h> 
#include <avr/interrupt.h>
#include <i2cmaster.h>
#include "eeprom.h"
#include "display.h"

#ifdef debug
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#endif




// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

unsigned long keys;

int ichbinaus=0;
int led_farbe=0;
unsigned int led_dimm=255;
//

#ifdef debug
int uart_putc(unsigned char c)
{
  while (!(UCSR1A & (1<<UDRIE1)))  
  {
  }                             
  UDR1 = c;                     
  return 0;
}

void uart_puts (char *s)
{
  while (*s)
  {   
    uart_putc(*s);
    s++;
  }
}

int inituart(void)
{
  //
  // Achtung, wir nutzen die 2. UART!
  UBRR1H = UBRR_VAL >> 8;
  UBRR1L = UBRR_VAL & 0xFF;
  UCSR1B |= (1<<TXEN);			  // UART TX einschalten
  UCSR1C =  (1 << UCSZ1) | (1 << UCSZ0);  // Asynchron 8N1 
	return 0;
}

unsigned char inttochar(unsigned int rein)
{
	//int myInt;
	unsigned char raus;
	raus = (unsigned char)rein;
	return raus;
	
}
#endif

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

int save2memory()
{
	//
	// Wichtig, hier werden Interrupts gesperrt!
	cli();
	
  //
	// EEPROM
	unsigned char IOReg;
	
	DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   //SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
	IOReg   = SPSR;                            //SPI Status und SPI Datenregister einmal auslesen
	IOReg   = SPDR;
	PORTB |= (1<<PB0);                         //ChipSelect aus
	
	unsigned char H_Add=0b00000000;    
  unsigned char M_Add=0b00000000;
	unsigned char L_Add=0b00000000;
	
	ByteWriteSPI(H_Add,L_Add,M_Add,memory);   //Variable test an Test Adresse schreiben
	return 0;
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

ISR (INT6_vect)
{
	#ifdef debug
	uart_puts("INT6\r\n");
	#endif
	keycheck();
}

ISR (INT7_vect)
{
	#ifdef debug
	uart_puts("INT7\r\n");
	#endif
	/*
	else if(byte0 == 251)
	{
		#ifdef debug
		uart_puts("mod\r\n");
		#endif
    if(mod == 1)
    {
			mod=2;
    }
    else if(mod == 2)
    {
			mod=3;
    }
    else if(mod == 3)
    {
			mod=4;
    }
    else if(mod == 4)
    {
			mod=1;
    }
		modulation(mod);
	}
	else if(byte0 == 247)
	{
		#ifdef debug
		uart_puts("step\r\n");
		#endif

	}

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

unsigned long keysauslesen()
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

int keycheck(void)
{
	keys=keysauslesen();
	#ifdef debug
	uint8_t string[20];
	uart_puts("Daten: ");
	sprintf(string,"%lX",keys);
	uart_puts(string);
	uart_puts("\r\n");
	#endif
	

	// 10. Bit
	// TX Anfang, PTT Taste ist gedrückt
	if((keys & 0x100) == 0)
	{
		#ifdef debug
		uart_puts("SW13\r\n");
    uart_puts("Dimmer\r\n");
		#endif
		if(led_dimm == 255)
		{
			led_dimm=128;
		}
		else if(led_dimm == 128)
		{
			led_dimm=64;
		}
		else
		{
			led_dimm=255;
		}
		led_helligkeit(led_dimm);
		
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
	/*
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
	*/
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
}


//
// sehr unelegant, muss mit einem Timer gemacht werden
// erstmal geht es nur um den HW Test ... ^^
/*
int scan(void)
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
*/
int rogerbeep()
{
	//
	// 2 Töne
	DDRB |= (1<<PB5); 
	TCCR1A = (1<<WGM10) | (1<<COM1A1); 
	TCCR1B = (1<<CS11) | (1<<CS10);
	OCR1A = 128-1; 
	_delay_ms(250);
	TCCR1A &= ~((1 << COM1A1) | (1 << WGM10)); 
	_delay_ms(100);
	TCCR1A = (1<<WGM10) | (1<<COM1A1); 
	_delay_ms(250);
	TCCR1A &= ~((1 << COM1A1) | (1 << WGM10));
	return 0;
}

ISR(ADC_vect)
{
	uint16_t x;
	//x += (ADCH<<8);
	// oder besser
	x = ADCW;
	char s[7];
	#ifdef debug
	uart_puts("Messwert: ");
	uart_puts( itoa( x, s, 10 ) );
	uart_puts("\r\n");
	#endif
}







int main(void) 
{
	cli();
	//_delay_ms(1000);
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
  //TCCR0 = (1<<CS01); // Prescaler 8
	TCCR0|=(1<<CS00) | (1<<CS01);
	
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
	led_helligkeit(led_dimm);
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
