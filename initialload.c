// initialload.c
// for NerdKits with ATmega168
// mrobbins@mit.edu

#define F_CPU 14745600

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <inttypes.h>

#include "../libnerdkits/delay.h"
#include "../libnerdkits/lcd.h"
#include <avr/sleep.h>


void getTemp();

void adc_init() {
	ADMUX = 0;
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
	ADCSRA |= (1<<ADSC);
}

uint16_t adc_read() {
	while (ADCSRA & (1<<ADSC)) {}
	uint16_t result = ADCL;
	uint16_t temp = ADCH;
	result = result + (temp<<8);

	ADCSRA |= (1<<ADSC);

	return result;
}

double sampleToFahrenheit(uint16_t sample) {
	return sample * (5000.0 / 1024.0 / 10.0);

}

double fahrenheitToCelsius(double f) {
	return (f - 32.0) * (5.0/9.0);
}

double celsiusToKelvin(double c) {
	return c + 273.0;
}

// PIN DEFINITIONS:
//
// PC4 -- LED anode

// we have to write our own interrupt vector handler..
ISR(PCINT1_vect) {
	// this code will be called anytime that PCINT18 switches
	//    (hi to lo, or lo to hi)
	//~ lcd_init();
	//~ lcd_home();
	//~ lcd_line_one();
	//~ lcd_write_string(PSTR("  XYZ "));
	getTemp();
}

void getTemp() {
	uint16_t last_sample = 0;
	double this_temp;
	double temp_avg;
	uint16_t i;
	temp_avg = 0.0;

	for (i=0; i<500; i++) {
		last_sample = adc_read();
		this_temp = sampleToFahrenheit(last_sample);
		temp_avg = temp_avg + this_temp/500.0;
	}

	double c = fahrenheitToCelsius(temp_avg);
	double k = celsiusToKelvin(c);

	// write message to LCD
	lcd_init();
	FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);
	lcd_home();
	lcd_write_string(PSTR("ADC: "));
	lcd_write_int16(last_sample);
	lcd_write_string(PSTR(" of 1024 "));
	lcd_line_two();

	fprintf_P(&lcd_stream, PSTR("%.2f"), temp_avg);
	lcd_write_data(0xdf);
	lcd_write_string(PSTR("F"));

	lcd_line_three();
	fprintf_P(&lcd_stream, PSTR("%.2f"), c);
	lcd_write_data(0xdf);
	lcd_write_string(PSTR("C"));

	lcd_line_four();
	fprintf_P(&lcd_stream, PSTR("%.2f"), k);
	lcd_write_data(0xdf);
	lcd_write_string(PSTR("K"));
}

int main() {

	// LED as output
	DDRC |= (1<<PC4);

	//enable internal pullup resistors
	PORTC |= (1 << PC3); //button

	// Make digital 4 (PCINT18/PD2) an input
	//pinMode(4, INPUT);

	// this is ATMEGA168 specific, see page 70 of datasheet

	// Pin change interrupt control register - enables interrupt vectors
	// Bit 2 = enable PC vector 2 (PCINT23..16)
	// Bit 1 = enable PC vector 1 (PCINT14..8)
	// Bit 0 = enable PC vector 0 (PCINT7..0)
	PCICR |= (1 << PCIE1);

	// Pin change mask registers decide which pins are enabled as triggers
	PCMSK1 |= (1 << PCINT11);

	// enable interrupts
	//interrupts();
	//sei();

	// turn on LED
	PORTC |= (1<<PC4);

	// fire up the LCD
	//~ lcd_init();
	//~ FILE lcd_stream = FDEV_SETUP_STREAM(lcd_putchar, 0, _FDEV_SETUP_WRITE);
	//~ lcd_home();
	adc_init();

	// print message to screen
	//			 20 columns wide:
	//                     01234567890123456789
	//~ lcd_line_one();
	//~ lcd_write_string(PSTR("  Congratulations!  "));
	//~ lcd_line_two();
	//~ lcd_write_string(PSTR("********************"));
	//~ lcd_line_three();
	//~ lcd_write_string(PSTR("  Your USB NerdKit  "));
	//~ lcd_line_four();
	//~ lcd_write_string(PSTR("    is alive! Apples"));

	// turn off that LED
	PORTC &= ~(1<<PC4);

	// busy loop
	while (1) {
		
		
		// turn on LED
		PORTC |= (1<<PC4);
		delay_ms(200);
		// turn off that LED
		PORTC &= ~(1<<PC4);
		delay_ms(200);

		lcd_init();
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		//set_sleep_mode(SLEEP_MODE_STANDBY);
		
		cli();
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		sei();

		// turn on LED
		PORTC |= (1<<PC4);
		delay_ms(200);
		// turn off that LED
		PORTC &= ~(1<<PC4);
		delay_ms(200);

		delay_ms(5000);
	}

	return 0;
}
