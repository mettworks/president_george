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

/*
Aufbau Array "memory":

0 Frequenz
1 Frequenz
2 Modulation
3 Step
4 NBANL
5 HICUT
6 Echo
7 RogerBeep
8 CB/HAM
9 An oder Aus

10 Beleuchtung Helligkeit Tasten
11 Beleuchtung Helligkeit Display
12 Beleuchtung Farbe

*/

extern unsigned int memory[6];

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
	
void ByteWriteSPI(unsigned char HighAdd, unsigned char LowAdd, unsigned int data)
{
	#ifdef debug
	uart_puts("\r\nBeginn ByteWriteSPI()\r\n");
	char string[20];
	uart_puts("Zieladresse: ");
	sprintf(string,"%u,",HighAdd);
	uart_puts(string);
	sprintf(string,"%u",LowAdd);
	uart_puts(string);
	uart_puts("\r\n");
	sprintf(string,"zu schreibendes Byte: %u\r\n",data);
	uart_puts(string);
	#endif
	WriteEnable();          //Schreiben aktivieren
	PORTB &= ~(1<<PB0);     //ChipSelect an
	_delay_us(10);
	WriteSPI(0x02);       //Opcode senden
	WriteSPI(HighAdd);   //Oberes Adressbyte senden
	WriteSPI(LowAdd);    //Unteres Adressbyte senden
	WriteSPI(data);
	PORTB |= (1<<PB0);   //ChipSelect aus
	SPIWIPPolling();     //Auf EEPROM warten
	#ifdef debug
	uart_puts("fertig geschrieben\r\n");
	#endif
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
	#ifdef debug
	uart_puts("\r\nBeginn ByteReadSPI()\r\n");
	char string[20];
	uart_puts("Zieladresse: ");
	sprintf(string,"%u,",HighAdd);
	uart_puts(string);
	sprintf(string,"%u",LowAdd);
	uart_puts(string);
	uart_puts("\r\n");
	sprintf(string,"gelesenes Byte: %u\r\n",data);
	uart_puts(string);
	#endif
	return data;            
}

int save2memory(void)
{
	//
	// Wichtig, hier werden Interrupts gesperrt!
	cli();
	unsigned char IOReg;
	DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   //SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
	IOReg   = SPSR;                            //SPI Status und SPI Datenregister einmal auslesen
	IOReg   = SPDR;
	PORTB |= (1<<PB0);                         //ChipSelect aus
	unsigned char H_Add=0b00000000;    
	unsigned char L_Add=0b00000000;
	ByteWriteSPI(H_Add,L_Add,memory[0]);
	H_Add=0b00000000;    
	L_Add=0b00000001;
	ByteWriteSPI(H_Add,L_Add,memory[1]);
	H_Add=0x0;    
	L_Add=0x2;
	ByteWriteSPI(H_Add,L_Add,memory[2]);
	return 0;
}

void read_memory(void)
{
	unsigned char IOReg;
	DDRB = (1<<PB0) | (1<<PB2) | (1<<PB1);      //SS (ChipSelect), MOSI und SCK als Output, MISO als Input
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);   	//SPI Enable und Master Mode, Sampling on Rising Edge, Clock Division 16
	IOReg   = SPSR;                            		//SPI Status und SPI Datenregister einmal auslesen
	IOReg   = SPDR;
	PORTB |= (1<<PB0);                         	//ChipSelect aus
	
	unsigned char H_Add=0b00000000;    						
	unsigned char L_Add=0b00000000;
	memory[0] = ByteReadSPI(H_Add,L_Add);
	H_Add=0b00000000;
	L_Add=0b00000001;
	memory[1] = ByteReadSPI(H_Add,L_Add);
	H_Add=0x0;
	L_Add=0x2;
	memory[2] = ByteReadSPI(H_Add,L_Add);
}