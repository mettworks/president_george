/*
 * eeprom.c
 *
 * Created: 27.05.2012 11:56:02
 *  Author: Stefan Wohlers
 * gefunden: http://www.mikrocontroller.net/topic/260261
 */ 
 
#define F_CPU 18432000UL
/*
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
	PORTB &= ~(1<<PB2);     //ChipSelect an
	_delay_us(10);
	WriteSPI(0x05);         //Opcode senden
	data = ReadSPI();      //Antwort empfangen
	PORTB |= (1<<PB2);    //ChipSelect aus
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
		//uart_puts("SPIWIPPolling()\r\n");
	} 
	while (status & 0x01);	//Wiederhole bis WIP Bit auf Null gesetzt wird
}
	
void ByteWriteSPI(unsigned char HighAdd, unsigned char LowAdd, unsigned char MidAdd, unsigned char data)	
{
	WriteEnable();          //Schreiben aktivieren
	PORTB &= ~(1<<PB0);     //ChipSelect an
	_delay_us(10);
	WriteSPI(0x02);       //Opcode senden
	WriteSPI(HighAdd);   //Oberes Adressbyte senden
	WriteSPI(MidAdd);    //Mittleres Adressbyte senden
	WriteSPI(LowAdd);    //Unteres Adressbyte senden
	WriteSPI(data);      //Datenbyte senden
	PORTB |= (1<<PB0);   //ChipSelect aus
	SPIWIPPolling();     //Auf EEPROM warten
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
	return data;            
}
*/

/*
 * eeprom.c
 *
 * Created: 27.05.2012 11:56:02
 *  Author: Stefan Wohlers
 */ 
#include <avr/io.h>
#include "eeprom.h"
#include <util/delay.h>


///////////////////////////////////////////////////////////////////////////////
	
	
	//SPI Functions
	
	unsigned char ReadSPI()
	
	{
		unsigned char data;
		
		SPDR = 0x00;                   //Dummy Byte senden um 8 Clock Pulse zu erzeugen
		
		while(!(SPSR & (1<<SPIF)));   //Warten bis Byte gesendet bzw. neues Byte empfangen wurde
		
		data = SPDR;                 //Empfangenes Byte in Variable data schreiben
		
		return data;                 
		
	}
	
	/////////////////////////////////////////////////////////////////////////////////
	
	
	void WriteSPI(unsigned char data)
	
	{
		SPDR = data;   //Byte ins Datenregister schreiben
		
		while(!(SPSR & (1<<SPIF)));
		
		
	}
	
	//////////////////////////////////////////////////////////////////////////////////
	
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
	
	/////////////////////////////////////////////////////////////////////////////////
	
	void WriteEnable()
	
	{
		PORTB &= ~(1<<PB0);    //ChipSelect an
		
		_delay_us(10);
		
		WriteSPI(0x06);       //Opcode senden
		
		PORTB |= (1<<PB0);    //ChipSelect aus
				
		
	}
	
	////////////////////////////////////////////////////////////////////////////////
	
	
	void WriteDisable()
	
	{
		PORTB &= ~(1<<PB0);  //ChipSelect an
		
		_delay_us(10);
		
		WriteSPI(0x04);     //Opcode senden
		
		PORTB |= (1<<PB0);  //ChipSelect aus
		
	}		
	
	/////////////////////////////////////////////////////////////////////////////////
	
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
				
		} while (status & 0x01);	//Wiederhole bis WIP Bit auf Null gesetzt wird

	}
	
	////////////////////////////////////////////////////////////////////////////////

	
	void ByteWriteSPI(unsigned char HighAdd, unsigned char LowAdd, unsigned char MidAdd, unsigned char data)
	
	{
		WriteEnable();          //Schreiben aktivieren
		
		PORTB &= ~(1<<PB0);     //ChipSelect an
		
		_delay_us(10);
		
		WriteSPI(0x02);       //Opcode senden
		
		WriteSPI(HighAdd);   //Oberes Adressbyte senden
		
		WriteSPI(MidAdd);    //Mittleres Adressbyte senden
		
		WriteSPI(LowAdd);    //Unteres Adressbyte senden
		
		WriteSPI(data);      //Datenbyte senden
		
		PORTB |= (1<<PB0);   //ChipSelect aus
		
		SPIWIPPolling();     //Auf EEPROM warten
	}
	
	/////////////////////////////////////////////////////////////////////////////////
	
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
		
	    return data;            
		
	}
	
	/////////////////////////////////////////////////////////////////////////////////////
	
