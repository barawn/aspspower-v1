/*
 * usci_uart.h
 *
 *
 * NOTE NOTE NOTE NOTE: This currently still only works for USCI A0. There's dumbass compiler bizarreness
 * going on preventing me from easily making an offset-based USCI handler like the EUSCI.
 *
 *
 *
 *
 *  Created on: Jul 21, 2015
 *      Author: barawn
 */

#ifndef USCI_UART_H_
#define USCI_UART_H_


#include <msp430.h>
#include "uart_base.h"

// All MSP430s with USCIs have one of these defined.
#if not defined(__MSP430_HAS_USCI_A0__) && not defined(__MSP430_HAS_USCI__)
#error USCI_UART not found in selected device.
#else
#if defined(__MSP430_BASEADDRESS_USCI_A0__)
#define USCI_BASE ((uint16_t) __MSP430_BASEADDRESS_USCI_A0__)
#else
#define USCI_BASE ((uint16_t) &UCA0CTL0)
#endif
#define USCI_OFFSET(x) ((uint16_t) (&x - USCI_BASE))
#define USCI_REG8( x ) (*((volatile uint8_t *) (USCI_OFFSET(x) + (uint16_t) CONFIG::BASE)))
#define USCI_REG16( x ) (*((volatile uint16_t *) (USCI_OFFSET(x) + (uint16_t) CONFIG::BASE)))
#endif

template<typename CONFIG>
class USCI_UART : public uart_base< USCI_UART<CONFIG>, CONFIG > {
public:
	USCI_UART() {}
	static void init_impl() {
		USCI_REG8(UCA0CTL1) = CONFIG::CTL1 | UCSWRST;
		USCI_REG8(UCA0CTL0) = CONFIG::CTL0;
		USCI_REG8(UCA0BR0) = CONFIG::BR0;
		USCI_REG8(UCA0BR1) = CONFIG::BR1;
		USCI_REG8(UCA0MCTL) = CONFIG::MCTL;
		USCI_REG8(UCA0CTL1) &= ~UCSWRST;
	}
	static void control_impl(bool enable) {
		if (enable) *CONFIG::IE |= CONFIG::RXIE;
		else *CONFIG::IE &= ~CONFIG::RXIE;
	}
	static bool sleep_check_impl() {
		// is the interrupt running?
		if (!(*CONFIG::IE & CONFIG::TXIE)) {
			// No. So is there anything available?
			if (*(USCI_UART::get_tx_rd_ptr()) !=
				*(USCI_UART::get_tx_wr_ptr())) {
				// Yes, so enable the interrupt.
				*CONFIG::IE |= CONFIG::TXIE;
			}
		}
		return true;
	}
	// This is here because of weirdness: I can't redeclare
	// a static member outside of the class, but I can't
	// include attributes in the actual function, so the
	// only way is to change the class definition that
	// each file sees.
	//
	// Here, the library-level code (COMPONENTS_DEFAULT)
	// sees 'weak' attributes, so their defaults can be overriden.
	// Everyone else sees strong, so their functions override.
#ifdef COMPONENTS_DEFAULT
	static void is_empty(lwevent *ev) __attribute__((weak));
	static void has_data(lwevent *ev) __attribute__((weak));
#else
	static void is_empty(lwevent *ev);
	static void has_data(lwevent *ev);
#endif
};


#endif /* USCI_UART_H_ */
