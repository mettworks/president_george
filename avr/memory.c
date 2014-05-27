/*
 * Credits:
 *  Author: Stefan Wohlers
 *  http://www.mikrocontroller.net/topic/260261
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif

extern unsigned char memory[MEM_SIZE];
extern int modus;
extern int vfo;
extern unsigned int ham_mod_a;
extern unsigned int ham_mod_b;

// TODO vfo global
void save_freq(unsigned long freq2tune,unsigned int vfo)
{
  uart_puts("save_freq()\r\n");
  if(vfo == 0)
  {
    memory[3] = freq2tune / 16777215;
    memory[2] = freq2tune / 65535;
    memory[1] = freq2tune / 256;
    memory[0] = freq2tune % 256;
  }
  else
  {
    memory[7] = freq2tune / 16777215;
    memory[6] = freq2tune / 65535;
    memory[5] = freq2tune / 256;
    memory[4] = freq2tune % 256;
  }
}

void save_ch(unsigned char ch)
{
  memory[8] = ch;
}

void save_mod(unsigned char mod)
{
  if(modus==0)
  {
    if(vfo == 0)
    {
      // xx00 0000
      memory[12] = (memory[12] & 0x3f) | (mod << 6);
      ham_mod_a = mod;
    }
    else
    {
      // 00xx 0000
      memory[12]=(memory[12] & 0xcf) | ( mod << 4);
      ham_mod_b = mod;
    }
  }
  else
  {
    // 0000 xx00
    memory[12]=(memory[12] & 0xf3) | ( mod << 2);
  }
}

void save_echo(unsigned int echo)
{
  if(modus == 0)
  {
    if(echo == 0)
    {
      memory[14] &= ~(1 << 1);
    }
    else
    {
      memory[14] |= (1 << 1);
    }
  }
  else
  {
    if(echo == 0)
    {
      memory[14] &= ~(1 << 0);
    }
    else
    {
      memory[14] |= (1 << 0);
    }
  }
}

void save_beep(unsigned int beep)
{
  if(modus == 0)
  {
    if(beep == 0)
    {
      memory[14] &= ~(1 << 3);
    }
    else
    {
      memory[14] |= (1 << 3);
    }
  }
  else
  {
    if(beep == 0)
    {
      memory[14] &= ~(1 << 2);
    }
    else
    {
      memory[14] |= (1 << 2);
    }
  }
}
unsigned char ReadSPI(void)
{
  unsigned char data;
  SPDR = 0x00;                   //Dummy Byte senden um 8 Clock Pulse zu erzeugen
  while(!(SPSR & (1<<SPIF)));   //Warten bis Byte gesendet bzw. neues Byte empfangen wurde
  data = SPDR;                 //Empfangenes Byte in Variable data schreiben
  return data;                 
}
	
void WriteSPI(unsigned char data)
{
  SPDR = data;   //Byte ins Datenregister schreiben
  while(!(SPSR & (1<<SPIF)));
}
	
unsigned char ReadStatus(void)
{
  unsigned char data;
  PORTB &= ~(1<<PB0);     //ChipSelect an
  _delay_us(10);
  WriteSPI(0x05);         //Opcode senden
  data = ReadSPI();      //Antwort empfangen
  PORTB |= (1<<PB0);    //ChipSelect aus
  return data;
}
	
void WriteEnable(void)
	
{
  PORTB &= ~(1<<PB0);    //ChipSelect an
  _delay_us(10);
  WriteSPI(0x06);       //Opcode senden
  PORTB |= (1<<PB0);    //ChipSelect aus
}
	
void WriteDisable(void)
{
  PORTB &= ~(1<<PB0);  //ChipSelect an
  _delay_us(10);
  WriteSPI(0x04);     //Opcode senden
  PORTB |= (1<<PB0);  //ChipSelect aus
}		
	
void SPIWIPPolling(void)
{
  unsigned char status=0;
  do 
  {
    PORTB &= ~(1<<PB0);      //ChipSelect an
    _delay_us(10);
    WriteSPI(0x05);          //Opcode senden
    status=ReadSPI();       //Antwort empfangen
    PORTB |= (1<<PB0);      //ChipSelect aus
  } 
  while (status & 0x01);	//Wiederhole bis WIP Bit auf Null gesetzt wird
}
	
void ByteWriteSPI(unsigned char HighAdd, unsigned char LowAdd, unsigned char data)
{
  WriteEnable();          //Schreiben aktivieren
  PORTB &= ~(1<<PB0);     //ChipSelect an
  _delay_us(10);
  WriteSPI(0x02);       //Opcode senden
  WriteSPI(HighAdd);   //Oberes Adressbyte senden
  WriteSPI(LowAdd);    //Unteres Adressbyte senden
  WriteSPI(data);
  PORTB |= (1<<PB0);   //ChipSelect aus
  SPIWIPPolling();     //Auf EEPROM warten
}
	
unsigned char ByteReadSPI(unsigned char HighAdd, unsigned char LowAdd)
{
  unsigned char data=0;
  PORTB &= ~(1<<PB0);     //ChipSelect an
  _delay_us(10);
  WriteSPI(0x03);         //Opcode senden
  WriteSPI(HighAdd);      //Oberes Adressbyte senden
  WriteSPI(LowAdd);       //Unteres Adressbyte senden
  data=ReadSPI();         //Datenbyte in Variable data schreiben
  PORTB |= (1<<PB0);      //ChipSelect aus
  return data;            
}

int save2memory(void)
{
  //
  // Wichtig, hier werden Interrupts gesperrt!
  cli();
  #ifdef debug
  uart_puts("save2memory():\r\n");
  #endif
  int i=0;
  unsigned char H_Add=0;    
  unsigned char L_Add=0;
  DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   //SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
  PORTB |= (1<<PB0);                         //ChipSelect aus
  while(i < MEM_SIZE)
  {
    ByteWriteSPI(H_Add,L_Add,memory[i]);
    #ifdef debug
    char string[20];
    uart_puts("H_Add: ");
    sprintf(string,"%u,",H_Add);
    uart_puts(string);
    uart_puts("L_Add: ");
    sprintf(string,"%u,",L_Add);
    uart_puts(string);
    uart_puts("Data: ");
    sprintf(string,"%u,",memory[i]);
    uart_puts(string);
    uart_puts("\r\n");
    #endif
    L_Add++;
    i++;
  }
  return 0;
}

void read_memory(void)
{
  #ifdef debug
  uart_puts("read_memory()\r\n");
  #endif
  int i=0;
  unsigned char H_Add=0;    
  unsigned char L_Add=0;
  DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   	//SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
  PORTB |= (1<<PB0);                         	//ChipSelect aus

  while(i < MEM_SIZE)
  {
    memory[i]=ByteReadSPI(H_Add,L_Add);
    #ifdef debug
    char string[20];
    uart_puts("H_Add: ");
    sprintf(string,"%u,",H_Add);
    uart_puts(string);
    uart_puts("L_Add: ");
    sprintf(string,"%u,",L_Add);
    uart_puts(string);
    uart_puts("Data: ");
    sprintf(string,"%u,",memory[i]);
    uart_puts(string);
    uart_puts("\r\n");
    #endif
    L_Add++;
    i++;
  }
  #ifdef debug
  uart_puts("read_memory() ENDE\r\n");
  #endif
}
void format_memory(void)
{
  unsigned int i=0;
  while(i < MEM_SIZE)
  {
    memory[i]=0;
    i++;
  }
  save2memory();
}


