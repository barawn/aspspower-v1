/*
 * usci_ser0_config.h
 *
 *  Created on: Jul 21, 2015
 *      Author: barawn
 */

#ifndef USCI_SER0_CONFIG_H_
#define USCI_SER0_CONFIG_H_

#include <stdint.h>
#include <msp430.h>
#include "uart_base.h"

#define UART_OBJECT( x ) USCI_Ser0_##x
//% \brief This is required for the ISR, since there's no way to include that in the Config class. Make sure BASE and VECTOR_LOCATION match!
//% Note that this can be left undefined: if so,
//% it's presumed that a shared ISR somewhere else jumps to it.
//% Forget to do that and you'll end up with ISR_TRAP handling it!
#define USCI_Ser0_RX_VECTOR_LOCATION	USCIAB0RX_VECTOR	//< RX vector. Leave undefined for a shared vector with USCI B.
#define USCI_Ser0_TX_VECTOR_LOCATION	USCIAB0TX_VECTOR	//< TX vector. Leave undefined for a shared vector with USCI B.

//% \brief Actual ring buffer pointers. Pointed to inside config class.
extern uart_base_pointers UART_OBJECT(pointers);
//% \brief Actual ring buffer RX storage. Pointed to inside config class.
extern uint8_t UART_OBJECT(rx_buffer[]);
//% \brief Actual ring buffer TX storage. Pointed to inside config class.
extern uint8_t UART_OBJECT(tx_buffer[]);
//% \brief Actual TX empty lwevent. Overridable.
extern lwevent UART_OBJECT(tx_empty_event);
//% \brief Actual RX data lwevent. Overridable.
extern lwevent UART_OBJECT(rx_data_event);
//% \brief Actual tx_empty store. Overridable.
extern lwevent_store_fifo UART_OBJECT(tx_empty_store);

//% \brief Configuration class for USCI_Ser0.
//%
//% Picking off the pointers is done to avoid MSP430 C compiler
//% stupidity (buf[*pidx] is more efficient than buf[p->idx]).
//% Collecting them in a class in the first place is done to reduce
//% the number of implementation-specific objects (e.g. EUSCI_Ser0_)
//% prefixed objects.
class UART_OBJECT(Config) {
public:
	const uint8_t TX_BUF_SIZE = 128;
	const uint8_t RX_BUF_SIZE = 16;
	const uint16_t BASE = (uint16_t) &UCA0CTL0;
	const uint8_t USCI_INDEX = 0;
	volatile uint8_t *const IE = &IE2;
	const uint8_t TXIE = UCA0TXIE;
	const uint8_t RXIE = UCA0RXIE;
	const uint8_t CTL1 = UCSSEL_2;
	const uint8_t CTL0 = 0;
	const uint8_t BR0 = 104;
	const uint8_t BR1 = 0;
	const uint8_t MCTL = UCBRS0;

	uart_base_pointers *const pointers = &UART_OBJECT(pointers);
	uint8_t *const rx_buffer = UART_OBJECT(rx_buffer);
	uint8_t *const tx_buffer = UART_OBJECT(tx_buffer);
	lwevent *const tx_empty_event = &UART_OBJECT(tx_empty_event);
	lwevent *const rx_data_event = &UART_OBJECT(rx_data_event);
	lwevent_store_fifo *const tx_empty_store = &UART_OBJECT(tx_empty_store);

	uint8_t *const rx_rd_ptr = &pointers->rx_rd;
	uint8_t *const rx_wr_ptr = &pointers->rx_wr;
	uint8_t *const tx_rd_ptr = &pointers->tx_rd;
	uint8_t *const tx_wr_ptr = &pointers->tx_wr;
};


//% Define this to exclude the default empty/data lwevents. Use this if you override the lwevents themselves. Note that NOT defining this still works,
//% but costs some RAM/code space. This is because the compiler can't tell that that event is unused, because it looks like the handler accesses it,
//% and the event also contains the handler. So it looks like the usage count of both is 1.
//#define USCI_SER0_NO_DEFAULT_LWEVENTS

//% \brief Define this to exclude the default tx_empty lwevent (automatically excluded if SER0_NO_DEFAULT_LWEVENTS is set)
//#define USCI_SER0_NO_DEFAULT_TX_EMPTY_LWEVENT

//% \brief Define this to exclude the default RX data lwevent (automatically excluded if SER0_NO_DEFAULT_LWEVENTS is set)
//#define USCI_SER0_NO_DEFAULT_RX_DATA_LWEVENT

#undef UART_OBJECT

#endif /* USCI_SER0_CONFIG_H_ */
