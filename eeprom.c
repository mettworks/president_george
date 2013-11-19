/*
 * eeprom.c
 *
 * Created: 27.05.2012 11:56:02
 *  Author: Stefan Wohlers
 * gefunden: http://www.mikrocontroller.net/topic/260261
 */ 
//#define debug

#define F_CPU 18432000UL
#include <avr/io.h>
#include "eeprom.h"
#include <util/delay.h>

unsigned char ReadSPI()
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
	
unsigned char ReadStatus()
{
	unsigned char data;
	PORTB &= ~(1<<PB0);     //ChipSelect an
	_delay_us(10);
	WriteSPI(0x05);         //Opcode senden
	data = ReadSPI();      //Antwort empfangen
	PORTB |= (1<<PB0);    //ChipSelect aus
	return data;
}
	
void WriteEnable()
	
{
	PORTB &= ~(1<<PB0);    //ChipSelect an
	_delay_us(10);
	WriteSPI(0x06);       //Opcode senden
	PORTB |= (1<<PB0);    //ChipSelect aus
}
	
void WriteDisable()
{
	PORTB &= ~(1<<PB0);  //ChipSelect an
	_delay_us(10);
	WriteSPI(0x04);     //Opcode senden
	PORTB |= (1<<PB0);  //ChipSelect aus
}		
	
void SPIWIPPolling()
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
	
void ByteWriteSPI(unsigned char HighAdd, unsigned char LowAdd, unsigned char MidAdd, unsigned char data[6])
{
	
	#ifdef debug
	uart_puts("\r\nBeginn ByteWriteSPI()\r\n");
	uint8_t string0[20];
	uint8_t string1[20];
	sprintf(string0,"zu schreibendes Byte0: %u\r\n",data[0]);
	uart_puts(string0);
	sprintf(string1,"zu schreibendes Byte1: %u\r\n",data[1]);
	uart_puts(string1);
	#endif
	
	WriteEnable();          //Schreiben aktivieren
	PORTB &= ~(1<<PB0);     //ChipSelect an
	_delay_us(10);
	WriteSPI(0x02);       //Opcode senden
	WriteSPI(HighAdd);   //Oberes Adressbyte senden
	WriteSPI(MidAdd);    //Mittleres Adressbyte senden
	WriteSPI(LowAdd);    //Unteres Adressbyte senden
	WriteSPI(data[0]);
	WriteSPI(data[1]); 
	WriteSPI(data[2]);
	WriteSPI(data[3]); 
	WriteSPI(data[4]);
	WriteSPI(data[5]); 
	WriteSPI(data[6]);
	PORTB |= (1<<PB0);   //ChipSelect aus
	SPIWIPPolling();     //Auf EEPROM warten
	#ifdef debug
	uart_puts("fertig geschrieben\r\n");
	#endif
}
	
unsigned char ByteReadSPI(unsigned char HighAdd, unsigned char LowAdd, unsigned char MidAdd)
{
	unsigned char data=0;
	PORTB &= ~(1<<PB0);     //ChipSelect an
	_delay_us(10);
	WriteSPI(0x03);         //Opcode senden
	WriteSPI(HighAdd);      //Oberes Adressbyte senden
	WriteSPI(MidAdd);       //Mittleres Adressbyte senden
	WriteSPI(LowAdd);       //Unteres Adressbyte senden
	data=ReadSPI();         //Datenbyte in Variable data schreiben
	PORTB |= (1<<PB0);      //ChipSelect aus
	#ifdef debug
	uart_puts("\r\nBeginn ByteReadSPI()\r\n");
	uint8_t string[20];
	uart_puts("Zieladresse: ");
	sprintf(string,"%u,",HighAdd);
	uart_puts(string);
	sprintf(string,"%u,",MidAdd);
	uart_puts(string);
	sprintf(string,"%u",LowAdd);
	uart_puts(string);
	uart_puts("\r\n");
	sprintf(string,"gelesenes Byte: %u\r\n",data);
	uart_puts(string);
	#endif
	return data;            
}