/*
 * usci_ser0.cpp
 *
 *  Created on: Jul 21, 2015
 *      Author: barawn
 */

// This causes all the overrideable handlers to have a 'weak' attribute.
#define COMPONENTS_DEFAULT

// Change these lines for a new UART. Everything else autorenames.
#include "usci_ser0.h"
#define UART_OBJECT( x ) USCI_Ser0_##x
typedef USCI_Ser0 This_UART;
#if defined(USCI_SER0_NO_DEFAULT_LWEVENTS) || defined(USCI_SER0_NO_DEFAULT_TX_EMPTY_LWEVENT)
#define THIS_UART_NO_TX_EMPTY_LWEVENT
#endif
#if defined(USCI_SER0_NO_DEFAULT_LWEVENTS) || defined(USCI_SER0_NO_DEFAULT_RX_DATA_LWEVENT)
#define THIS_UART_NO_RX_DATA_LWEVENT
#endif

/////////////////////////////
// UART objects.
/////////////////////////////
uart_base_pointers UART_OBJECT(pointers);
uint8_t UART_OBJECT(tx_buffer)[UART_OBJECT(Config)::TX_BUF_SIZE];
uint8_t UART_OBJECT(rx_buffer)[UART_OBJECT(Config)::RX_BUF_SIZE];
lwevent_store_fifo USCI_Ser0_tx_empty_store;

// Prototype function for empty, if user does not include it.
void This_UART::is_empty(lwevent *ev) {
	ev->clear();
}
// Prototype function for data receive, if user does not include it.
void This_UART::has_data(lwevent *ev) {
	ev->clear();
}
#if not defined(THIS_UART_NO_TX_EMPTY_LWEVENT)
lwevent UART_OBJECT(tx_empty_event) __attribute__((weak)) ((lwevent_handler_t) This_UART::is_empty, (lwevent *) lwevent::LWEVENT_WAITING);
#endif
#if not defined(THIS_UART_NO_RX_DATA_LWEVENT)
lwevent UART_OBJECT(rx_data_event) __attribute__((weak)) ((lwevent_handler_t) This_UART::has_data,  (lwevent *) lwevent::LWEVENT_WAITING);
#endif

