;
; I AM STILL LOOKING FOR A GOOD WAY TO DO THIS.
;
	.cdecls CPP, LIST, "usci_ser0_asm.h"
	.define	USCI_Ser0_Config_enum.USCI_INDEX, usci_uart_index
	.include "usci_magic.asm"
	.include "lwevent_asm.asm"

USCI_Ser0_RX_ISR:
	push	r15
	mov.b	&USCI_Ser0_pointers.rx_wr, r15
	mov.b	&usci_uart_RXBUF, USCI_Ser0_rx_buffer(r15)
	inc.b	r15
	bic.b	#USCI_Ser0_Config_enum.RX_BUF_SIZE, r15
	mov.b	r15, &USCI_Ser0_pointers.rx_wr
	pop		r15
	isr_lwevent_post	USCI_Ser0_rx_data_event
	bic.w	#(LPM3), 0(SP)
	reti
USCI_Ser0_TX_ISR:
	; New byte to transmit?
	cmp.b	&USCI_Ser0_pointers.tx_wr, &USCI_Ser0_pointers.tx_rd
	; If equal, there isn't.
	jeq		USCI_Ser0_TX_ISR_empty
	; Not equal, so there is one. Copy it to TXBUF, increment pointer, and continue.
	push	r15
	mov.b	&USCI_Ser0_pointers.tx_rd, r15
	mov.b	USCI_Ser0_tx_buffer(r15), &usci_uart_TXBUF
	inc.b	r15
	bic.b	#USCI_Ser0_Config_enum.TX_BUF_SIZE, r15
	mov.b	r15, &USCI_Ser0_pointers.tx_rd
	pop		r15
	reti
USCI_Ser0_TX_ISR_empty:
	bic.b	#usci_uart_TXIE, &usci_uart_IE
	isr_lwevent_post_wakeup	USCI_Ser0_tx_empty_event, 0
	reti


	.sect 	USCI_Ser0_RX_VECTOR_LOCATION
	.short	USCI_Ser0_RX_ISR
	.sect	USCI_Ser0_TX_VECTOR_LOCATION
	.short	USCI_Ser0_TX_ISR



