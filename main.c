#define F_CPU 7372800UL
#define BAUD 9600UL
#define debug

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h> 
#include <avr/interrupt.h>
// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.
#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

unsigned int wert;

/*
  HTH, richtige Reihenfolge:
  S/RF 1. Bit
  SWR
  CAL
  MUTE
  PA
  T/R
  ?
  DIM

  ECHO 
  HICUT
  NB/ANL
  USB
  LSB
  AM
  FM
  MOD 16.Bit
  
*/
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
#endif

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
//
// Treiber
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
//
// PLL
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
int main(void) 
{
  //
  // Definitionen
  //unsigned int wert;

  //
  // Ein und Ausgaenge
  // PD2 ist Ein/Aus	    -> Ausgang
  // PD3 ist TX vom Mikro   -> Eingang, ohne Pullup
  // PD5 ist		    -> Eingang, mit Pullup
  // PD6 ist		    -> Eingang, mit Pullup  
  // PD7 ist		    -> Eingang, mit Pullup
  // PB0 ist		    -> Eingang, mit Pullup
  // PC2 ist		    -> Ausgang
  // PC3 ist		    -> Ausgang
  // PC4 ist		    -> Ausgang
  // PC5 ist		    -> Ausgang
  
  // PD2
  DDRD |= (1<<PD2);
  PORTD |= (1<<PD2);	// einschalten
  // PD3 
  DDRD &= ~(1<<PD3);
  // PD5
  DDRD &= ~(1<<PD5);	// Eingang
  PORTD |= (1<<PD5);	// internen Pullup aktivieren
  // PD6
  DDRD &= ~(1<<PD6);	// Eingang
  PORTD |= (1<<PD6);	// internen Pullup aktivieren
  // PD7
  DDRD &= ~(1<<PD7);	// Eingang
  PORTD |= (1<<PD7);	// internen Pullup aktivieren
  // PB0
  DDRB &= ~(1<<PB0);	// Eingang
  PORTB |= (1<<PB0);	// internen Pullup aktivieren
  // PC2
  DDRC |= (1<<PC2);
   // PC3
  DDRC |= (1<<PC3);
  // PC4
  DDRC |= (1<<PC4);
  // PC5
  DDRC |= (1<<PC5);
 
  //_delay_ms(5000);
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  #endif
  
  wert=0;
  //wert |= (1 << TREIBER_FM);
  //
  // Achtung, MUTE muss auf 1 stehen!!
  wert |= (1 << TREIBER_MUTE);
  treiber(wert); 

  //
  // initial auf FM
  int mod = 1;
  modulation(mod);
  
  unsigned int freq = 29100;
  unsigned int step = 5;
  tune(freq,step);
  
  //
  // Endlos Schleife fuer die Taster
  while(1)
  {
    if(!(PIND & (1 << PIND5)))
    {
      _delay_ms(150);
      #ifdef debug
      uart_puts("up\r\n");
      #endif
      freq=freq+step;
      tune(freq,step);
    }
    if(!(PIND & (1 << PIND6)))
    {
      _delay_ms(150);
      #ifdef debug
      uart_puts("down\r\n");
      #endif
      freq=freq-step;
      tune(freq,step);
    }
    if(!(PIND & (1 << PIND7)))
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
    if(!(PINB & (1 << PINB0)))
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
    if(!(PIND & (1 << PIND3)))
    {
      _delay_ms(100);
      #ifdef debug
      uart_puts("TX\r\n");
      #endif
      wert |= (1 << TREIBER_TR);
      treiber(wert); 
      while(1)
      { 
	if(PIND & (1 << PIND3))
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
