/*
 * uart_base.h
 *
 * This follows the CRTP-type implementation of the PDQ graphics
 * library ( https://github.com/XarkLabs/PDQ_GFX_Libs ) and creates a UART base class, for use
 * with the lwevent framework.
 *
 * Peripheral base classes basically follow the same methodology here:
 * 1) Templated base class for overall functionality: UART base, I2C master base, I2C slave base, etc.
 * 2) Templated implementation class for implementation-specific functions.
 * 3) Configuration class which contains linkages (const pointers) to static elements, as well as instantiation-specific
 *    details.
 *
 * The reason for the last step is complicated. The C++ compiler is smart enough to recognize that
 * when you define a const pointer, it can just go ahead and replace that with the symbol for the
 * object it pointed to (since, well, that's what it is anyway). But the assembler's cdecls parser
 * is not smart enough to do that: it just skips the const pointer initialization, and then it wonders
 * why the linker hasn't created a symbol for that object.
 *
 * Plus, the cdecls parser is just not that useful for C++. Sure, it creates globals for you, but does it
 * with mangled C++ names. So you have to find the mangled names, and then reimport them in. THIS IS NOT USEFUL.
 *
 *  Created on: Jun 26, 2015
 *      Author: barawn
 */

#ifndef UART_BASE_H_
#define UART_BASE_H_

#include <stdint.h>
#include "lwevent.h"

//% \brief Collection of buffer pointers.
//%
//% Buffers themselves are not here, because their storage size is unknown.
typedef struct uart_base_pointers {
	uint8_t rx_rd;
	uint8_t rx_wr;
	uint8_t tx_rd;
	uint8_t tx_wr;
} uart_base_pointers;

template<class T, class CONFIG>
class uart_base {
public:
	uart_base() {}
	void init() {
		*tx_rd_ptr = 0;
		*tx_wr_ptr = 0;
		*rx_rd_ptr = 0;
		*rx_wr_ptr = 0;
		T::init_impl();
	}
	void control(bool enable) {
		T::control_impl(enable);
	}
	// Dump all read bytes.
	void dump() {
		*rx_rd_ptr = *rx_wr_ptr;
	}
	// Get number of bytes available to read.
	uint8_t rx_available() {
		return (*rx_wr_ptr - *rx_rd_ptr) % RX_BUF_SIZE;
	}
	// Get number of bytes available to transmit.
	uint8_t tx_available() {
		return (*tx_rd_ptr - *tx_wr_ptr - 1) % TX_BUF_SIZE;
	}
	//% \brief Receive a character. Don't call unless you know one is available! (via rx_available)
	//%
	//% The tmpptr bit here is to defeat poor TI optimization: since rx_rd_ptr is declared volatile,
	//% it assigns a register to tmpptr (which it had to do *anyway*, which is why the optimization is poor)
	//% and then uses that register throughout the whole calculation.
	//%
	//% With 'volatile' missing and/or tmpptr not used, the C compiler fetches rx_rd_ptr *again*, rather
	//% than using the local copy. The volatile declaration prevents that, since it could have changed.
	uint8_t rx() {
		uint8_t tmp, tmpptr;
		tmpptr = *rx_rd_ptr;
		tmp = rx_buffer[tmpptr++];
		tmpptr = tmpptr % T::RX_BUF_SIZE;
		*rx_rd_ptr = tmpptr;
		return tmp;
	}
	//% Transmit a character. Note there is no overflow protection (so use tx_available if this is a worry)
	//% The tmp bit here is to defeat poor TI optimization: since tx_wr_ptr is declared volatile,
	//% it assigns a register to tmp (which it had to do *anyway*, which is why the optimization is poor)
	//% and then uses that register throughout the whole calculation.
	//%
	//% With 'volatile' missing and/or tmp not used, the C compiler fetches tx_wr_ptr *again*, rather
	//% than using the local copy. The volatile declaration prevents that, since it could have changed.
	void tx(uint8_t txc) {
		uint8_t tmp;
		tmp = *tx_wr_ptr;
		tx_buffer[tmp++] = txc;
		tmp = tmp % TX_BUF_SIZE;
		*tx_wr_ptr = tmp;
	}

	//% Transmit a character, but this is a virtual function. This is for use with the
	//% xprintf function (pulled in through the lwprintf class).
	virtual void write(uint8_t txc);

	//% Sleep check. This is called before sleeping for each module. Modules cannot interact with each other here.
	bool sleep_check() {
		return T::sleep_check_impl();
	}
	//% Add a lwevent to post when the TX fifo empties.
	void post_when_tx_empty(lwevent *p) {
		tx_empty_store->store(p);
	}
	static volatile uint8_t *const get_tx_rd_ptr() { return tx_rd_ptr; }
	static volatile uint8_t *const get_tx_wr_ptr() { return tx_wr_ptr; }
	static volatile uint8_t *const get_rx_rd_ptr() { return rx_rd_ptr; }
	static volatile uint8_t *const get_rx_wr_ptr() { return rx_wr_ptr; }
	static uint8_t *const get_rx_buffer() { return rx_buffer; }
	static uint8_t *const get_tx_buffer() { return tx_buffer; }

	uint8_t const RX_BUF_SIZE = CONFIG::RX_BUF_SIZE;
	uint8_t const TX_BUF_SIZE = CONFIG::TX_BUF_SIZE;
	volatile uint8_t *const tx_wr_ptr = CONFIG::tx_wr_ptr;
	volatile uint8_t *const tx_rd_ptr = CONFIG::tx_rd_ptr;
	volatile uint8_t *const rx_wr_ptr = CONFIG::rx_wr_ptr;
	volatile uint8_t *const rx_rd_ptr = CONFIG::rx_rd_ptr;
	uint8_t *const rx_buffer = CONFIG::rx_buffer;
	uint8_t *const tx_buffer = CONFIG::tx_buffer;
	lwevent *const tx_empty_event = CONFIG::tx_empty_event;
	lwevent *const rx_data_event = CONFIG::rx_data_event;
	lwevent_store_fifo *const tx_empty_store = CONFIG::tx_empty_store;
};

template<class T, class CONFIG>
void uart_base<T,CONFIG>::write(uint8_t txc) {
	tx(txc);
}

#endif /* UART_BASE_H_ */
