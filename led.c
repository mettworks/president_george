#include <avr/io.h>
#include "led.h"
#include "i2c.h"
#ifdef debug
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#endif


void led_helligkeit1(unsigned int led_dimm)
{
	#ifdef debug
	uart_puts("led_helligkeit1(): Anfang\r\n");
	#endif
	led_pwm(1,led_dimm);
	led_pwm(2,led_dimm);
	led_pwm(3,led_dimm);
	led_pwm(4,led_dimm);
	led_pwm(5,led_dimm);
	led_pwm(6,led_dimm);
	led_pwm(7,led_dimm);
	led_pwm(8,led_dimm);
	led_pwm(9,led_dimm);
	led_pwm(10,led_dimm);
}

void led_helligkeit2(unsigned int led_dimm)
{
	#ifdef debug
	uart_puts("led_helligkeit2(): Anfang\r\n");
	#endif
	led_pwm(11,led_dimm);
	led_pwm(12, led_dimm);
}

void init_led(void)
{
	#ifdef debug
	uart_puts("init_led(): Anfang\r\n");
	#endif
	i2c_start_wait(0xc0); // TLC59116 Slave Adresse ->C0 hex
	#ifdef debug
	uart_puts("init_led(): Start Wait Ende\r\n");
	#endif
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
	#ifdef debug
	uart_puts("init_led(): Ende\r\n");
	#endif
}

void led_pwm(int led, int pwm)
{
	i2c_start_wait(0xc0);
	i2c_write(0x01 + led);
	i2c_write(pwm);
	i2c_stop();
}

void led_color(int color)
{
	/*
	if (color == 0 )
	{
		*/
	  #ifdef debug
		uart_puts("LED Farbe: Grün\r\n");
		#endif
		PORTC |= (1<<PC0);	  		// Grün 1
		PORTC &= ~(1<<PC1);     	// Rot 0
		/*
	}
	else if (color == 1)
	{
		#ifdef debug
		uart_puts("LED Farbe: Rot\r\n");
		#endif
		PORTC |= (1<<PC1);	  		// Rot 1
		PORTC &= ~(1<<PC0);     	// Grün 0
	}
	*/
}