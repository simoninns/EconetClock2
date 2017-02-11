/************************************************************************
	main.c

    Acorn Econet Clock 2
    Copyright (C) 2017 Simon Inns

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Email: simon.inns@gmail.com

************************************************************************/

// Internal clock @ 8MHz - This code is using the internal oscillator
// so don't expect the clock speed to be too accurate.  In my tests the
// output was within about 10 KHz of the expected value - close enough
// for this application.
#define F_CPU 8000000

// Global includes
#include <avr/io.h>
#include <util/delay.h>

// ---------------------------------------------------------------------------------

// The period and mark must be a whole number in the range of 0 to 255 and the
// mark *MUST* be lower than the period.  Due to the clock speed of the ATtiny45
// you have to take the required microseconds (uS) and divide it by 0.03125 (and
// round up) to get the desired value.
//
// For example: if you want the default Acorn setting of a 200 KHz clock this would
// require a period of 5 uS (1,000,000 uS per second / 200,000 Hz = 5 uS period) and
// the default mark of 1 uS:
//
// Period of 5uS = 5uS / 0.03125 = 160
// Mark of 1uS = 1uS / 0.03125 = 32
//


// The following clock configurations can be selected by setting the DIP switch
// as shown (0 = Off, 1 = On):
//
// DIP SW	Period (uS)	Mark (us)	Psetting	Msetting
// 000		5.00		1.000		160			32			
// 001		4.00		0.750		128			24			
// 010		4.00		0.500		128			16			
// 011		3.00		0.500		 96			16			
// 100		3.00		0.250		 96			 8			
// 101		2.00		0.500		 64			16			
// 110		2.00		0.250		 64			 8			
// 111		2.00		0.125		 64			 4			

#define ECONET_000_PERIOD	160
#define ECONET_001_PERIOD	128
#define ECONET_010_PERIOD	128
#define ECONET_011_PERIOD	 96
#define ECONET_100_PERIOD	 96
#define ECONET_101_PERIOD	 64
#define ECONET_110_PERIOD	 64
#define ECONET_111_PERIOD	 64

// Note: Due to an error in the original clock schematics (from Acorn)
//   the PCB design has reversed clock +ve and -ve signals.  To compensate
//   the 'mark' is inverted by subtracting it from the overall period.
#define ECONET_000_MARK		160-32
#define ECONET_001_MARK		128-24
#define ECONET_010_MARK		128-16
#define ECONET_011_MARK		 96-16
#define ECONET_100_MARK		 96- 8
#define ECONET_101_MARK		 64-16
#define ECONET_110_MARK		 64- 8
#define ECONET_111_MARK		 64- 4

// ---------------------------------------------------------------------------------
// Hardware definitions

// Clock out positive (PB3 - !OC1B)
#define CLOCKOUTP_PORT	PORTB
#define CLOCKOUTP_PIN	PINB
#define CLOCKOUTP_DDR	DDRB
#define CLOCKOUTP		(1 << 3)

// Clock out negative (PB4 - OC1B)
#define CLOCKOUTN_PORT	PORTB
#define CLOCKOUTN_PIN	PINB
#define CLOCKOUTN_DDR	DDRB
#define CLOCKOUTN		(1 << 4)

// DIP Switch 1 (LSB)
#define DIPSW1_PORT	PORTB
#define DIPSW1_PIN	PINB
#define DIPSW1_DDR	DDRB
#define DIPSW1		(1 << 0)

// DIP Switch 2
#define DIPSW2_PORT	PORTB
#define DIPSW2_PIN	PINB
#define DIPSW2_DDR	DDRB
#define DIPSW2		(1 << 1)

// DIP Switch 3 (MSB)
#define DIPSW3_PORT	PORTB
#define DIPSW3_PIN	PINB
#define DIPSW3_DDR	DDRB
#define DIPSW3		(1 << 2)

int main(void)
{
	// Variable to store the current and previous DIP switch settings (0-7)
	int8_t currentDipSwitch = 0;
	int8_t previousDipSwitch = 0;
	
	// Set the CPU clock prescaler (overrides CKDIV8 fuse if set)
	CLKPR=(1 << CLKPCE);
	CLKPR=(0 << CLKPCE) | (0 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);
	
	// Configure the clock out negative pin
	CLOCKOUTN_DDR |= CLOCKOUTN;			// Set direction to output
	CLOCKOUTN_PORT &= ~(CLOCKOUTN);		// Set output to off
	
	// Configure the clock out positive pin
	CLOCKOUTP_DDR |= CLOCKOUTP;			// Set direction to output
	CLOCKOUTP_PORT &= ~(CLOCKOUTP);		// Set output to off
	
	// Set DIP Switch pins to input and turn on weak pull-ups
	DIPSW1_DDR &= ~DIPSW1;
	DIPSW2_DDR &= ~DIPSW2;
	DIPSW3_DDR &= ~DIPSW3;
	
	DIPSW1_PORT |= DIPSW1;
	DIPSW2_PORT |= DIPSW2;
	DIPSW3_PORT |= DIPSW3;
	
	// Positive (non-inverted) PWM output on OC1B (PB4)
	// Negative (inverted) PWM output on !OC1B (PB3)
	
	// Configure the PWM module - I've split the configuration over
	// several lines and left in the bits set to zero to make it
	// easier to understand what's going on...  Since we are just
	// setting up the hardware PWM it doesn't matter if it takes a
	// couple more cycles to get going.
	//
	// The comments attempt to mirror the datasheet so you can see
	// what's going on -  PWM configuration on AVRs is confusing at best :)

	// Initialize Timer/Counter1 for asynchronous mode:

	// To set Timer/Counter1 in asynchronous mode first enable PLL
	// and then wait 100 uS for PLL to stabilize. Next, poll
	// the PLOCK bit until it is set and then set the PCKE bit.
	
	// Enable PLL
	PLLCSR |= (1 << PLLE);

	// Wait 100uS for the PLL to stabilize
	_delay_ms(100);

	// Wait for the PLOCK bit to be set in PLLCSR
	while ((PLLCSR & (1 << PLOCK)) == 0x00);
	
	// Enable the PCK clock for asynchronous mode
	// (i.e. route it to timer/counter1)
	PLLCSR |= (1 << PCKE);
	
	// We will use Timer/Counter1 in PWM mode
	
	// Disconnect OC1A as we don't need it
	TCCR1 |= (0 << COM1A1) | (0 << COM1A0);
	TCCR1 |= (1 << PWM1A); // do we need this?
	
	// Connect OC1B and its inverted friend !OC1B
	//
	// We want COM1B1 = 0 and COM1B0 = 1:
	// OC1B cleared on compare match. Set when TCNT1 = &00
	// OC1B set on compare match. Cleared when TCNT1 = &00
	GTCCR |= (0 << COM1B1) | (1 << COM1B0);
	
	// Now we have to enable the clock and set the prescaler value
	// (prescale is the amount the clock is divided before being
	// used as an input to Timer/Counter1).  The faster the clock
	// ticks, the more accurately we can time the period and mark
	// needed for the Econet clock, but the overall period is then
	// less (since the timer is 8-bit and can only span a maximum
	// of 255 ticks).  So we want a prescale value as low as possible
	// but not so low that we can't support the maximum period.
	//
	// The default clock frequency is 200 KHz with a mark of 1uS and
	// a space of 4uS (meaning the overall period is 5uS).  Since there
	// are 1,000,000uS in a second this means our clock frequency is 
	// 1,000,000 / 5us = 200,000 Hz or 200KHz.
	//
	// We want to support a minimum period of 0.25uS and a maximum
	// period of 8.0uS (i.e. 4000 KHz to 125 KHz).
	//
	// The PLL clock is 64MHz (i.e. 64,000 KHz) meaning that each tick
	// is 1,000,000uS / 64,000,000 Hz = 0.015625 uS meaning our maximum
	// period is 3.984375 uS (about 251 KHz).  Since we need a maximum
	// period of 200 KHz we can divide the prescaler by 2 which halves
	// our resolution (i.e. accuracy).
	//
	// At PCK/2 the clock is 32 MHz meaning each tick is 1,000,000 uS /
	// 32,000,000 Hz = 0.03125 uS meaning our maximum period is 7.96875
	// uS (about 125.5 KHz) - perfect :)
	// 
	
	// Enable Timer/Counter1 in Asynchronous mode with PCK/2 prescaler
	TCCR1 |= (0 << CS13) | (0 << CS12) | (1 << CS11) | (0 << CS10);
	
	// Enable Pulse Width Modulator B
	GTCCR |= (1 << PWM1B);
	
	// Mark is set by OCR1B and the overall period is set by OCR1C
	// Default setting is DIP switch = 0
	OCR1B = ECONET_000_MARK;
	OCR1C = ECONET_000_PERIOD;
	
    // Since the PWM hardware is running the only thing the main loop
	// does is monitor the DIP switch settings and set the clock
	// when it changes.
    while (1) 
    {
		// Read the current DIP switch setting
		currentDipSwitch = 0;
		
		if (!(DIPSW1_PIN & DIPSW1)) currentDipSwitch += 1;
		if (!(DIPSW2_PIN & DIPSW2)) currentDipSwitch += 2;
		if (!(DIPSW3_PIN & DIPSW3)) currentDipSwitch += 4;
		
		// Has the DIP switch setting changed?
		if (currentDipSwitch != previousDipSwitch)
		{
			switch(currentDipSwitch)
			{
				case 0:
					OCR1B = ECONET_000_MARK;
					OCR1C = ECONET_000_PERIOD;
					break;
					
				case 1:
					OCR1B = ECONET_001_MARK;
					OCR1C = ECONET_001_PERIOD;
					break;
				
				case 2:
					OCR1B = ECONET_010_MARK;
					OCR1C = ECONET_010_PERIOD;
					break;
				
				case 3:
					OCR1B = ECONET_011_MARK;
					OCR1C = ECONET_011_PERIOD;
					break;
				
				case 4:
					OCR1B = ECONET_100_MARK;
					OCR1C = ECONET_100_PERIOD;
					break;
				
				case 5:
					OCR1B = ECONET_101_MARK;
					OCR1C = ECONET_101_PERIOD;
					break;
				
				case 6:
					OCR1B = ECONET_110_MARK;
					OCR1C = ECONET_110_PERIOD;
					break;
				
				case 7:
					OCR1B = ECONET_111_MARK;
					OCR1C = ECONET_111_PERIOD;
					break;
			}
			
			previousDipSwitch = currentDipSwitch;
		}
	}
}



