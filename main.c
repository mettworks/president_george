/*
1. Version die was sinnvolles tut...

Orginal CPU wurde abgeklemmt. Der Atmega schaltet das Geraet auf Kanal 23 FM ein. An PC3 und PC2 (die beiden LE's) befinden 10kOhm Widerstaende nach VCC. 

Noch Quick and Dirty!

*/

#define F_CPU 7372800UL
#define BAUD 9600UL
#define CLOCK 2000

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h> 
#include <avr/interrupt.h>

/*
// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

int uart_putc(unsigned char c)
{
  while (!(UCSRA & (1<<UDRE)))  
  {
  }                             
  UDR = c;                     
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
  UBRRH = UBRR_VAL >> 8;
  UBRRL = UBRR_VAL & 0xFF;
 
  UCSRB |= (1<<TXEN);  // UART TX einschalten
  UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);  // Asynchron 8N1 
}
SIGNAL (SIG_OVERFLOW1)
{
  PORTC ^= (1 << PC5);
}
*/
int wait(void)
{
  _delay_us(40);
  return 0;
}
int data0(void)
{
  PORTC &= ~(1<<PC4);	  // Data 0
  _delay_us(14);
  PORTC |= (1<<PC5);	  // Clock 1
  _delay_us(14);
  PORTC &= ~(1<<PC5);	  // Clock 0
  return 0;
}
int data1(void)
{
  PORTC |= (1<<PC4);	  // Data 1
  _delay_us(14); 
  PORTC |= (1<<PC5);	  // Clock 1
  _delay_us(14);
  PORTC &= ~(1<<PC5);     // Clock 0
  PORTC &= ~(1<<PC4);	  // Data 0
  return 0;
}

int begin0(void)
{
  PORTC &= ~(1<<PC5);	// Clock 0
  PORTC &= ~(1<<PC4);   // Data 0
  PORTC &= ~(1<<PC3);   // LE 0
  _delay_us(56);
  return 0;
}
int end0(void)
{
  PORTC &= ~(1<<PC5);    // Clock 1
  PORTC |= (1<<PC3);	// LE1
  _delay_us(14);
  PORTC &= ~(1<<PC3);
   
  return 0;
}

int begin1(void)
{
  PORTC &= ~(1<<PC5);	// Clock 0
  PORTC &= ~(1<<PC4);   // Data 0
  PORTC &= ~(1<<PC2);   // LE 0
  _delay_us(56);
  return 0;
}
int end1(void)
{
  PORTC &= ~(1<<PC5);    // Clock 1
  PORTC |= (1<<PC2);	// LE1
  _delay_us(14);
  PORTC &= ~(1<<PC2);
  return 0;
}


int main(void) 
{
  /*
  while(1)
  {
  }
  */
  DDRC=0xff;
  PORTC=0x0;

  _delay_ms(5000);

  /*
  inituart();
  while(1)
  {
    uart_puts("blabla\r\n");
    _delay_ms(500);
  }
  */
  /* 
  while(1)
  {
    _delay_us(1);
    // ein
    PORTC |= (1<<PC5);
    _delay_us(1);
    // aus
    PORTC &= ~(1<<PC5);
  }
  */
  /*
  DDRC=(1<<PC5);
  TCCR1B |= 0x01;
  TIMSK |= (1 << TOIE1);
  sei();
  _delay_ms(5000);
  cli();
  */

  //DDRC=(1<<PC2);  // LE PLL
  //DDRC=(1<<PC3);  // LE FUER DIE AUSGAENGE
  //DDRC=(1<<PC5);  // Clock
  //DDRC=(1<<PC4);  // Data
  /*
  while(1)
  {
    begin();
    data1();
    data0();
    end();
    _delay_ms(1000);
  }
  */
  /*
  while(1)
  {
    // hell
    begin0();
    data1();
    data0();
    data0();
    data1();
    data0();
    data0();
    data1();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    data0();
    data0();
    end0();
    _delay_ms(2000);

    //dunkel
    begin0();
    data1();
    data0();
    data0();
    data1();
    data0();
    data0();
    data1();
    data1();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    data0();
    data0();
    end0();
    _delay_ms(2000);
  }
  */
    // FM 
    begin0();
    // 01000001
    data0();
    data1();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    // 00000001
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    // 0
    data0();
    end0();

    // nochmal 
    begin0();
    // 10010010
    data1();
    data0();
    data0();
    data1();
    data0();
    data0();
    data1();
    data0();
    // 00000010
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    data0();
    end0();    

    // PLL auf Kanal 22 rasten lassen
    begin1();
    // 10000001
    data1();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    // 00000000
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    // 01000001
    data0();
    data1();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    // 00000001
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    // 01001001
    data0();
    data1();
    data0();
    data0();
    data1();
    data0();
    data0();
    data1();
    // 00000001
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    // 00000100
    data0();
    data0();
    data0();
    data0();
    data0();
    data1();
    data0();
    data0();
    // 00000000
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    data0();
    // 01
    data0();
    data1();
    end1();

    // immer noch PLL...
    begin1();
    // 00011101
    data0();
    data0();
    data0();
    data1();
    data1();
    data1();
    data0();
    data1();
    // 10100110
    data1();
    data0();
    data1();
    data0();
    data0();
    data1();
    data1();
    data0();
    // 0
    data0();
    end1();

  return 0;
} 
