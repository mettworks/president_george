#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define EEPROM ((void *)0x32)

int press (char ausgang)
{
  _delay_ms(100);
  PORTC |= (1<<ausgang);
  _delay_ms(100);
  PORTC &= ~(1<<ausgang);
  return 0;
}

//
// wenn sich beim PD2 etwas tut, wird ein Interrupt ausgelöst
// dann hätte sich die Spannung geändert, d.h. Gerät wurde aus oder eingeschaltet
ISR(INT0_vect)
{
  //
  // wenn das Gerät stromlos wird, kann u.U. der falsche Status gespeichert werden
  // das Gerät geht ja aus und wenn das Ding hier noch bis zum speichern kommt...
  // daher 2 Sekunden warten...
  _delay_ms(2000);
  if ( PIND & (1<<PIND2) )
  {
    // ausgeschaltet
    eeprom_write_byte(EEPROM, 0);
  }
  else
  {
    // eingeschaltet
    eeprom_write_byte(EEPROM, 1);
  }
}

int main (void)
{
  cli();
  //
  // es wird 4 Mal die Program Taste gedrückt
  press(PC0);
  press(PC0);
  press(PC0);
  press(PC0);	    // Program
  //
  // einmal Power zum einschalten (gehört noch zum umgehen der Code Sperre)
  press(PC1);	    // Power
  //
  // Echo und RB einmal drücken weil wir es können... ;-)
  press(PC2);	    // Echo
  press(PC3);	    // Beep
 
  //_delay_ms(500);

  //
  // wenn das Gerät zuletzt aus war, schalten wir es wieder aus
  // da die Code Sperre umgangen ist, reicht ein Druck auf die Power Taste
  if(eeprom_read_byte(EEPROM) == 0)
  {
    press(PC1);
  }

  //
  // PD wird ein Eingang und der interne Pull-Up wird aktiviert
  DDRD  &= ~(1<<PD2);
  PORTD |= (1<<PD2);

  //
  // Interrupts scharfschalten
  MCUCR |= (1 << ISC00);
  GIMSK |= (1 << INT0);
  sei();

  //
  // Däumchen drehen... 
  while(1)
  {
  }
  return 0;
}
