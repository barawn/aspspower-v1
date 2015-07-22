If0:	.if	usci_uart_index = 0
		.define		UCA0RXBUF,	usci_uart_RXBUF
		.define		UCA0TXBUF,	usci_uart_TXBUF
		.define		IE2,		usci_uart_IE
		.define		UCA0TXIE,	usci_uart_TXIE
		.elseif usci_uart_index = 1
		.define		UCA1RXBUF,	usci_uart_RXBUF
		.define		UCA1TXBUF,	usci_uart_TXBUF
		.define		UC1IE,		usci_uart_IE
		.define		UCA1TXIE,	usci_uart_TXIE
		.endif
