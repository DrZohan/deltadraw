#include "i2c.h"

static const char SLV_Addr = 0x90;

static volatile int I2C_State = 0;
extern volatile char I2C_Data;

void i2c_setup() {
	USICTL0 = USIPE6+USIPE7+USISWRST;    // Port & USI mode setup
	USICTL1 = USII2C+USIIE+USISTTIE;     // Enable I2C mode & USI interrupts
	USICKCTL = USICKPL;                  // Setup clock polarity
	USICNT |= USIIFGCC;                  // Disable automatic clear control
	USICTL0 &= ~USISWRST;                // Enable USI
	USICTL1 &= ~USIIFG;                  // Clear pending flag
	_EINT();
}

//******************************************************
// USI interrupt service routine
//******************************************************
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX (void) {
	if (USICTL1 & USISTTIFG) {             // Start entry?
		P1OUT |= 0x01;                     // LED on: Sequence start
		I2C_State = 2;                     // Enter 1st state on start
	}

	switch(I2C_State) {
		case 0: //Idle, should not get here
			break;

		case 2: //RX Address
			USICNT = (USICNT & 0xE0) + 0x08; // Bit counter = 8, RX Address
			USICTL1 &= ~USISTTIFG;   // Clear start flag
			I2C_State = 4;           // Go to next state: check address
			break;

		case 4: // Process Address and send (N)Ack
			USICTL0 |= USIOE;        // SDA = output
			if (USISRL == (SLV_Addr | 0x1)) { // Address match?
				USISRL = 0x00;         // Send Ack
				P1OUT &= ~0x01;        // LED off
				I2C_State = 8;         // Go to next state: TX data
			}
			else {
				USISRL = 0xFF;         // Send NAck
				P1OUT |= 0x01;         // LED on: error
				I2C_State = 6;         // Go to next state: prep for next Start
			}
			USICNT |= 0x01;          // Bit counter = 1, send (N)Ack bit
			break;

		case 6: // Prep for Start condition
			USICTL0 &= ~USIOE;       // SDA = input
			I2C_State = 0;           // Reset state machine
			break;

		case 8: // Send Data byte
			USICTL0 |= USIOE;        // SDA = output
			USISRL = I2C_Data;       // Send data byte
			USICNT |=  0x08;         // Bit counter = 8, TX data
			I2C_State = 10;          // Go to next state: receive (N)Ack
			break;

		case 10:// Receive Data (N)Ack
			USICTL0 &= ~USIOE;       // SDA = input
			USICNT |= 0x01;          // Bit counter = 1, receive (N)Ack
			I2C_State = 12;          // Go to next state: check (N)Ack
			break;

		case 12:// Process Data Ack/NAck
			if (USISRL & 0x01) {      // If Nack received...
				P1OUT |= 0x01;         // LED on: error
			}
			else {                    // Ack received
				P1OUT &= ~0x01;        // LED off
			}
			// Prep for Start condition
			USICTL0 &= ~USIOE;       // SDA = input
			I2C_State = 0;           // Reset state machine
			break;
	}

	USICTL1 &= ~USIIFG;                  // Clear pending flags
}