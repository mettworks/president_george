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
  DDRC=0xff;
  PORTC=0x0;
  //einschalten
  DDRD=2;		// D2 als Ausgang   Bit 2 ist 1
  PORTD |= (1<<PD2);	// einschalten

  // PortD3 wird Eingang, ohne Pullup
  //DDRD &= ~(1<<PD3);
  //PORTD |= (1<<PD3);

  _delay_ms(5000);
  #ifdef debug
  inituart();
  uart_puts("\r\n\r\n");
  #endif

  // FM
  // TODO: Belegung der Bits ist noch falsch!!
  begin0();
  // 10010010
  data1();    // DIM?
  data0();    // ?
  data0();    // TX
  data1();    // PA
  data0();    // MUTE
  data0();    // CAL
  data1();    // SWR
  data0();    // S/RF
  // 00000010
  data0();    // ECHO
  data0();    // HI/CUT
  data0();    // NB/ANL
  data0();    // USB
  data0();    // LSB
  data0();    // AM
  data1();    // FM
  data0();    // 
  end0();  
    
  unsigned int freq = 29100;
  unsigned int step = 5;
  tune(freq,step);
  
  //
  // Endlos Schleife fuer die Taster
  while(1)
  {
    if(!(PIND & (1 << PIND3)))
    {
      _delay_ms(100);
      uart_puts("TX\r\n");
      // TODO: das geht noch deutlich eleganter, aber erst mal testen.... :-)
      begin0();
      data1();    
      data0();   
      data0();  
      data1();   
      data0();  
      data1();	  // <<< ---- Bit fuer TX 
      data1();    
      data0();    
      data0();    
      data0();   
      data0();   
      data0();    
      data0();    
      data0();   
      data1();   
      data0();   
      end0(); 
      while(1)
      { 
	if(PIND & (1 << PIND3))
	{    
	  _delay_ms(100); 
	  uart_puts("RX\r\n");
	  begin0();
	  data1(); 
	  data0();  
	  data0();   
	  data1();    
	  data0();    
	  data0();    // <<< ---- Bit fuer TX
	  data1();    
	  data0();    
	  data0();    
	  data0();    
	  data0();  
	  data0(); 
	  data0();    
	  data0();   
	  data1();  
	  data0();  
	  end0();  
	  break;
	}
      }
    }
  } 
  return 0;
} 
