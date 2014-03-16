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

extern unsigned int memory[MEM_SIZE];

unsigned char ReadSPI(void)
{
  unsigned char data;
  SPDR = 0x00;                   //Dummy Byte senden um 8 Clock Pulse zu erzeugen
  while(!(SPSR & (1<<SPIF)));   //Warten bis Byte gesendet bzw. neues Byte empfangen wurde
  data = SPDR;                 //Empfangenes Byte in Variable data schreiben
  return data;                 
}
	
void WriteSPI(unsigned int data)
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
	
void ByteWriteSPI(unsigned int HighAdd, unsigned int LowAdd, unsigned int data)
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
  unsigned int H_Add=0;    
  unsigned int L_Add=0;
  unsigned char IOReg;
  DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   //SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
  IOReg = SPSR;                            //SPI Status und SPI Datenregister einmal auslesen
  IOReg = SPDR;
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
  unsigned int H_Add=0;    
  unsigned int L_Add=0;
  unsigned char IOReg;
  DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   	//SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
  IOReg = SPSR;                            		//SPI Status und SPI Datenregister einmal auslesen
  IOReg = SPDR;
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
