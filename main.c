/*
avrdude -p atmega128 -P /dev/ttyACM0 -c stk500v2 -v -Uefuse:w:0xFF:m -U hfuse:w:0xC9:m -U lfuse:w:0xDF:m
*/

#define F_CPU 18432000UL
#define BAUD 115200UL
#define debug

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h> 
#include <avr/interrupt.h>
#include <i2cmaster.h>
#include "eeprom.h"

#ifdef debug
#include <stdio.h>
#include <stdint.h>
#endif


// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

unsigned int wert;
//
// Bits fuer den Treiberbaustein
#define TREIBER_MOD 0  
#define TREIBER_FM 1
#define TREIBER_AM 2
#define TREIBER_LSB 3
#define TREIBER_USB 4
#define TREIBER_NBANL 5
#define TREIBER_HICUT 6
#define TREIBER_ECHO 7

#define TREIBER_DIM 8
#define TREIBER_BIT9 9
#define TREIBER_TR 10
#define TREIBER_PA 11
#define TREIBER_MUTE 12
#define TREIBER_CAL 13
#define TREIBER_SWR 14
#define TREIBER_SRF 15


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

void inituart()
{
  //
  // Achtung, wir nutzen die 2. UART!
  UBRR1H = UBRR_VAL >> 8;
  UBRR1L = UBRR_VAL & 0xFF;
  UCSR1B |= (1<<TXEN);			  // UART TX einschalten
  UCSR1C =  (1 << UCSZ1) | (1 << UCSZ0);  // Asynchron 8N1 
}

unsigned char inttochar(unsigned int rein)
{
	//int myInt;
	unsigned char raus;
	raus = (unsigned char)rein;
	return raus;
	
}
#endif

int wait(void)
{
  _delay_us(40);
  return 0;
}
int data0(void)
{
  PORTD &= ~(1<<PD4);	  // Data 0
  _delay_us(14);
  PORTD |= (1<<PD5);	  // Clock 1
  _delay_us(14);
  PORTD &= ~(1<<PD5);	  // Clock 0
  return 0;
}
int data1(void)
{
  PORTD |= (1<<PD4);	  // Data 1
  _delay_us(14); 
  PORTD |= (1<<PD5);	  // Clock 1
  _delay_us(14);
  PORTD &= ~(1<<PD5);     // Clock 0
  PORTD &= ~(1<<PD4);	  // Data 0
  return 0;
}
//
// Treiber
int begin0(void)
{
  PORTD &= ~(1<<PD5);	// Clock 0
  PORTD &= ~(1<<PD4);   // Data 0
  PORTD &= ~(1<<PD7);   // LE 0
  _delay_us(56);
  return 0;
}
int end0(void)
{
  PORTD &= ~(1<<PD5);    // Clock 1
  PORTD |= (1<<PD7);	// LE1
  _delay_us(14);
  PORTD &= ~(1<<PD7);
  return 0;
}
//
// PLL
int begin1(void)
{
  PORTD &= ~(1<<PD5);	// Clock 0
  PORTD &= ~(1<<PD4);   // Data 0
  PORTD &= ~(1<<PD6);   // LE 0
  _delay_us(56);
  return 0;
}
int end1(void)
{
  PORTD &= ~(1<<PD5);    // Clock 1
  PORTD |= (1<<PD6);	// LE1
  _delay_us(14);
  PORTD &= ~(1<<PD6);
  return 0;
}

int treiber(unsigned int wert)
{
  #ifdef debug 
  uart_puts("Treiber\r\n");
  uart_puts("Bitfolge fuer Treiberbaustein:\t");
  #endif
  begin0();
  int index[16];
  int i;
  for(i=0; wert > 0; i++)
  {
    index[i]=wert%2;
    wert=wert/2;
  }
  for(;i < 16; i++)
  {
    index[i]=0;
  }

  for (; i > 0; i--)
  {
    if(index[i-1] == 0)
    {
      #ifdef debug
      uart_puts("0");
      #endif
      data0();
    }
    else
    {
      #ifdef debug
      uart_puts("1");
      #endif
      data1();
    }
  }
  end0();
  #ifdef debug
  uart_puts("\r\n");
  #endif

} 

int modulation(unsigned int mod)
{
  #ifdef debug 
  uart_puts("Modulation: ");
  #endif
  if(mod == 1)
  {
    // 1 FM
    #ifdef debug 
    uart_puts("FM");
    #endif 
    wert |= (1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 2)
  {
    // 2 AM
    #ifdef debug 
    uart_puts("AM"); 
    #endif
    wert |= (1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 3)
  {
    // 3 USB
    #ifdef debug 
    uart_puts("USB");
    #endif
    wert |= (1 << TREIBER_USB);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_LSB);
  }
  else if(mod == 4)
  {
    // 4 LSB
    #ifdef debug 
    uart_puts("LSB");
    #endif
    wert |= (1 << TREIBER_LSB);
    wert &= ~(1 << TREIBER_FM);
    wert &= ~(1 << TREIBER_AM);
    wert &= ~(1 << TREIBER_USB);
  }
  else
  {
    #ifdef debug 
    uart_puts("unbekannte modulation!");
    #endif
  }
  #ifdef debug 
  uart_puts("\r\n");
  treiber(wert);
  #endif
}

int tune(unsigned int freq,unsigned int step)
{
  begin1();
  //
  // Festlegung Kanalraster
  // Referenzquarz ist 10240kHz
  // 10240 / 2048 = 5, also der Teiler von 2048 waere dann ein Kanalraster von 5khz
  // 10240 / step = teiler
  // die Bitfolge muss 17 Bits lang sein
  #ifdef debug 
  uart_puts("Tuning\r\n");
  uart_puts("Frequenz: ");
  char text[10];
  utoa(freq,text,10);
  uart_puts(text);
  uart_puts("kHz\r\n");
  uart_puts("Step: ");
  utoa(step,text,10);
  uart_puts(text);
  uart_puts("kHz\r\n");
  uart_puts("Bitfolge fuer Kanalraster bzw. Referenz:\t");
  #endif
  unsigned int teiler_ref; 
  // TODO: define !
  teiler_ref=10240/step;
  int index[16];
  int i;
  for(i=0; teiler_ref > 0; i++)
  {
    index[i]=teiler_ref%2;
    teiler_ref=teiler_ref/2;
  }
  for(;i < 16; i++)
  {
    index[i]=0;
  }
  for (; i > 0; i--)
  {
    if(index[i-1] == 0)
    {
      #ifdef debug
      uart_puts("0");
      #endif
      data0();
    }
    else
    {
      #ifdef debug
      uart_puts("1");
      #endif
      data1();
    }
  }
  //
  // Achtung, Abschlussbit!
  data1();
  #ifdef debug
  uart_puts("1");
  uart_puts("\r\n");
  #endif
  end1();
  //
  // Festlegen des Teilers fuer die andere Seite
  // N Paket:
  // stellt den Teiler von der Sollfrequenz ein. 
  // (27255 + 10695) / 7590 = 5
  #ifdef debug
  uart_puts("Bitfolge fuer Teiler Sollfrequenz:\t\t");
  #endif
  begin1();
  unsigned int teiler_soll; 
  unsigned int teiler_soll_temp=freq+10695;
  teiler_soll=teiler_soll_temp/step;
  int index_soll[24];
  int j;
  for(j=0; teiler_soll > 0; j++)
  {
    index_soll[j]=teiler_soll%2;
    teiler_soll=teiler_soll/2;
  }
  for(;j < 24; j++)
  {
    index_soll[j]=0;
  }
  for (; j > 0; j--)
  {
    if(index_soll[j-1] == 0)
    {
      #ifdef debug
      uart_puts("0");
      #endif
      data0();
    }
    else
    {
      #ifdef debug
      uart_puts("1");
      #endif 
      data1();
    }
  }
  // 
  // Achtung, Abschlussbit!
  data0();
  #ifdef debug
  uart_puts("0");  
  uart_puts("\r\n");
  #endif
  end1();
  return 0;
}

int init_led()
{
	i2c_start_wait(0xc0); // TLC59116 Slave Adresse ->C0 hex
  i2c_write(0x80);  // autoincrement ab Register 0h

  i2c_write(0x00);  // Register 00 /  Mode1  
  i2c_write(0x00);  // Register 01 /  Mode2 

  i2c_write(0x00);  // Register 02 /  PWM LED 1    // Default alle PWM auf 0
  i2c_write(0x00);  // Register 03 /  PWM LED 2    
  i2c_write(0x00);  // Register 04 /  PWM LED 3
  i2c_write(0x00);  // Register 05 /  PWM LED 4
  i2c_write(0x00);  // Register 06 /  PWM LED 5
  i2c_write(0x00);  // Register 07 /  PWM LED 6
  i2c_write(0x00);  // Register 08 /  PWM LED 7
  i2c_write(0x00);  // Register 09 /  PWM LED 8
  i2c_write(0x00);  // Register 0A /  PWM LED 9
  i2c_write(0x00);  // Register 0B /  PWM LED 10
  i2c_write(0x00);  // Register 0C /  PWM LED 11
  i2c_write(0x00);  // Register 0D /  PWM LED 12
  i2c_write(0x00);  // Register 0E /  PWM LED 13
  i2c_write(0x00);  // Register 0F /  PWM LED 14
  i2c_write(0x00);  // Register 10 /  PWM LED 15
  i2c_write(0x00);  // Register 11 /  PWM LED 16  // Default alle PWM auf 0

  i2c_write(0xFF);  // Register 12 /  Group duty cycle control
  i2c_write(0x00);  // Register 13 /  Group frequency
  i2c_write(0xAA);  // Register 14 /  LED output state 0  // Default alle LEDs auf PWM
  i2c_write(0xAA);  // Register 15 /  LED output state 1  // Default alle LEDs auf PWM
  i2c_write(0xAA);  // Register 16 /  LED output state 2  // Default alle LEDs auf PWM
  i2c_write(0xAA);  // Register 17 /  LED output state 3  // Default alle LEDs auf PWM
  i2c_write(0x00);  // Register 18 /  I2C bus subaddress 1
  i2c_write(0x00);  // Register 19 /  I2C bus subaddress 2
  i2c_write(0x00);  // Register 1A /  I2C bus subaddress 3
  i2c_write(0x00);  // Register 1B /  All Call I2C bus address
  i2c_write(0xFF);  // Register 1C /  IREF configuration  
  i2c_stop();  // I2C-Stop
}

int led_pwm(int led, int pwm)
{
	i2c_start_wait(0xc0);
	i2c_write(0x01 + led);
	i2c_write(pwm);
	i2c_stop();
}

int led_color(int color)
{
	if (color == 0 )
	{
	  #ifdef debug
		uart_puts("LED Farbe: Rot\r\n");
		#endif
		PORTG |= (1<<PG0);	  		// Rot 1
		PORTG &= ~(1<<PG1);     	// Grün 0
	}
	else if (color == 1)
	{
		#ifdef debug
		uart_puts("LED Farbe: Gruen\r\n");
		#endif
		PORTG |= (1<<PG1);	  		// Grün 1
		PORTG &= ~(1<<PG0);     	// Rot 0
	}
}

ISR (INT4_vect)
{
	cli();
	while(1)
	{
		// erstmal zum testen, macht sich gut mit dem Logikanalyzer... :-)
		PORTG |= (1<<PG0);	
		_delay_ms(1);
		PORTG &= ~(1<<PG0);
		_delay_ms(1);
	}
}

int main(void) 
{
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
	uart_puts("Beginn main()\r\n");
	#endif
  //
  // Definitionen
  //unsigned int wert;
	
	//
	// Interrupts
	// INT4 wird bei fallender Flanke ausgelöst -> VCC weg
	DDRE &= ~(1<<PE4);	// Eingang
	EICRB |= (1<< ISC41);
	EIMSK |= (1 << INT4);
	sei();

	//
	// EEPROM
	unsigned char IOReg;
	
	DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   //SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
	IOReg   = SPSR;                            //SPI Status und SPI Datenregister einmal auslesen
	IOReg   = SPDR;
	PORTB |= (1<<PB0);                         //ChipSelect aus
	
	unsigned char out;
	unsigned char test=99;
	unsigned char H_Add=0b00000000;    //Test Adresse
  unsigned char M_Add=0b00000000;
	unsigned char L_Add=0b00000110;
	
	ByteWriteSPI(H_Add,L_Add,M_Add,test);   //Variable test an Test Adresse schreiben
	_delay_ms(100);
	out = ByteReadSPI(H_Add,L_Add,M_Add);  //Byte an Test Adresse auslesen und der Variablen out übergeben

  //
  // Ein und Ausgaenge
  // PE2 ist Ein/Aus	    	-> Ausgang		      
  // PE3 ist TX vom Mikro   -> Eingang, ohne Pullup  
  // PA3 ist Taster 1				-> Eingang, mit Pullup    
  // PA4 ist Taster 2				-> Eingang, mit Pullup    
  // PA5 ist Taster 3				-> Eingang, mit Pullup    
  // PA6 ist Taster 4				-> Eingang, mit Pullup    
  // PD6 ist Latch PLL      -> Ausgang		     
  // PD7 ist Latch Treiber  -> Ausgang		     
  // PD4 ist Data	    			-> Ausgang		   
  // PD5 ist Clock	    		-> Ausgang		     
	// PG0 ist LED Rot				-> Ausgang
	// PG1 ist LED Grün 			-> Ausgang
  
  // PE2
  DDRE |= (1<<PE2);
  PORTE |= (1<<PE2);	// einschalten
  // PE3 
  DDRE &= ~(1<<PE3);
  // PA3
  DDRA &= ~(1<<PA3);	// Eingang
  PORTA |= (1<<PA3);	// internen Pullup aktivieren
  // PA4
  DDRA &= ~(1<<PA4);	// Eingang
  PORTA |= (1<<PA4);	// internen Pullup aktivieren
  // PA5
  DDRA &= ~(1<<PA5);	// Eingang
  PORTA |= (1<<PA5);	// internen Pullup aktivieren
  // PA6
  DDRA &= ~(1<<PA6);	// Eingang
  PORTA |= (1<<PA6);	// internen Pullup aktivieren
  // PD6
  DDRD |= (1<<PD6);
  // PD7
  DDRD |= (1<<PD7);
  // PD4
  DDRD |= (1<<PD4);
  // PD5
  DDRD |= (1<<PD5);
	// PG0
	DDRG |= (1<<PG0);
	// PG1
	DDRG |= (1<<PG1);
 

  i2c_init();

    
  i2c_start_wait(0x70);    // Adresse, alle Bits auf 0 UND das R/W Bit!
  i2c_write(0xe0);  // ??   // IMHO Device Select 0
  i2c_write(0xcf);	  // multiplex 1100 1111
  i2c_write(0xFB);   // Bank select 2	    11111011 
  i2c_write(0xF0);   // Blink select (0xF0= off/0xF2= on) 
  i2c_write(0);

  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);

  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);

  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);

  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);
  i2c_write(0xff);

  i2c_stop();

	init_led();
	led_pwm(1,255);
	led_pwm(2,255);
	led_pwm(3,255);
	led_pwm(4,255);
	led_pwm(5,255);
	led_pwm(6,255);
	led_pwm(7,255);
	led_pwm(8,255);
	led_pwm(9,255);
	led_pwm(10,255);

	led_color(0);
		/*
	while(1)
	{
		led_color(0);
		_delay_ms(1000);
		led_color(1);
		_delay_ms(1000);
	}
	*/
	/*
    //delay_ms(2000);
    #ifdef debug
    uart_puts("Ende\r\n");
    #endif
    while(1)
    {
    }
  }
  */
  wert=0;
  //wert |= (1 << TREIBER_FM);
  //
  // Achtung, MUTE muss auf 1 stehen!!
  wert |= (1 << TREIBER_MUTE);
  // TEST
  wert |= (0 << TREIBER_ECHO);

  treiber(wert); 

  //
  // initial auf FM
  int mod = 1;
  modulation(mod);
  
  unsigned int freq = 28100;
  unsigned int step = 5;
  tune(freq,step);
  //
  // Endlos Schleife fuer die Taster
  while(1)
  {
    if(!(PINA & (1 << PINA3)))
    {
      _delay_ms(150);
      #ifdef debug
      uart_puts("up\r\n");
      #endif
      freq=freq+step;
      tune(freq,step);
    }
    if(!(PINA & (1 << PINA4)))
    {
      _delay_ms(150);
      #ifdef debug
      uart_puts("down\r\n");
      #endif
      freq=freq-step;
      tune(freq,step);
    }
    if(!(PINA & (1 << PINA5)))
    {
      _delay_ms(150);
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
    if(!(PINA & (1 << PINA6)))
    {
      _delay_ms(150);
      #ifdef debug
      uart_puts("step\r\n");
      if(step == 1)
      {
	step=5;
      }
      else if(step == 5)
      {
	step=10;
      }
      else
      {
	step=1;
      }
      tune(freq,step);
      #endif
    }
    if(!(PINE & (1 << PINE3)))
    {
      _delay_ms(100);
      #ifdef debug
      uart_puts("TX\r\n");
      #endif
      wert |= (1 << TREIBER_TR);
      treiber(wert); 
      while(1)
      { 
	if(PINE & (1 << PINE3))
	{    
	  _delay_ms(100); 
	  #ifdef debug
	  uart_puts("RX\r\n");
	  #endif
	  wert &= ~(1 << TREIBER_TR);
	  treiber(wert); 
	  break;
	}
      }
    }
  } 
  return 0;
} 
