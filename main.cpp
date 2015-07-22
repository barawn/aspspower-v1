#include <msp430.h> 
#include <stdint.h>
#include "lwevent.h"
#include "components.h"
#include "lwprintf.h"

#define DEVICE_INFO_SIZE 64
#pragma DATA_SECTION(".infoB")
uint8_t device_info[DEVICE_INFO_SIZE];
#pragma NOINIT
uint8_t device_info_buffer[DEVICE_INFO_SIZE];
#define DEVICE_INFO_KEY 0xA5
uint8_t device_info_wr_ptr = 0;

#define MAX_ADC 7
#define MAX_TEMP 3

#pragma DATA_SECTION(".infoC")
const uint16_t adc_map[MAX_ADC] = {
		INCH_0,
		INCH_1,
		INCH_6,
		INCH_14,
		INCH_3,
		INCH_11,
		INCH_10
};

// ASPS Power Pinout
// P1.1: OUT0 output/BSL transmit
// P1.2: \RST_ASPSDAQ
// P1.4: UPIVS_CLK
// P1.5: UPIVS_SDO
// P1.6: LED
// P1.7: LED
// P2.0/A0: 15V sense 0
// P2.1/A1: 15V sense 1
// P2.2: CCI0B input/BSL receive
// P2.3/A3: 15V mon
// P3.1: SDA
// P3.2: SCL
// P3.6/A6: 15V sense 2
// P4.0: EN0
// P4.1: EN1
// P4.2: EN2
// P4.3: EN3
// P4.5/A14: 15V sense 3

// sensors:
// sense 0 A0
// sense 1 A1
// sense 2 A6
// sense 3
// 15v mon
// my temp
// 3v3 mon
// vin mon
// tmp422 tmp
// ext tmp 1
// ext tmp 2
// that's 11 total sensors
// 7 are onboard: they're 0-6
// requesting 7 gets you "all onboards"
// 8,9,10 are the TMP422
// requesting 11 gets you "all temps"
// 12 gets you vin_mon

// xxxx xxxx
//       yyy = requested onboard housekeeping
//    z z    = requested temperature
//   w		 = requested vin_mon
// so 0xFF gets everything (so does 0x3F)




// world's dumbest commanding protocol
// <Hx>			- get housekeeping
// <h.....>		- return housekeeping, maybe more than 1
// <Dx>			- get binary housekeeping
// <d.....>		- return binary housekeeping
// <C#>			- control lines
// <c#>			- return control state
// <R#>			- read at address
// <r#>			- value at address
// <P#>			- set pointer to address
// <p#>			- return pointer
// <W#>
// <w#>
// Everything's single threaded right now, so no being stupid and asking
// for additional commands before responses.

void handle_command();
uint16_t read_onboard(uint8_t idx);

#pragma RETAIN
void Ser0::is_empty(lwevent *ev) {
	ev->clear();
	if (!Ser0::tx_empty_store->empty()) {
		queue1.post_store(*Ser0::tx_empty_store);
	}
}

enum e_Ser0_Parser_State {
	STATE_IDLE = 0,
	STATE_COMMAND = 1,
	STATE_ARGUMENT = 2,
	STATE_CHECK = 3
};

static enum e_Ser0_Parser_State Ser0_Parser_State = STATE_IDLE;
__attribute__((noinit)) uint8_t Ser0_command;
__attribute__((noinit)) uint8_t Ser0_argument;

#pragma RETAIN
void Ser0::has_data(lwevent *ev) {
	ev->clear();
	while (uart0.rx_available()) {
		switch (Ser0_Parser_State) {
			case STATE_IDLE:
				if (uart0.rx() == '<') Ser0_Parser_State = STATE_COMMAND;
				break;
			case STATE_COMMAND:
				Ser0_command = uart0.rx();
				Ser0_Parser_State = STATE_ARGUMENT;
				break;
			case STATE_ARGUMENT:
				Ser0_argument = uart0.rx();
				Ser0_Parser_State = STATE_CHECK;
				break;
			case STATE_CHECK:
				if (uart0.rx() == '>') handle_command();
				Ser0_Parser_State = STATE_IDLE;
				break;
		}
	}
}

void handle_controls(uint8_t ctrl_mask) {
	uint8_t tmp, tmp2;

	tmp = ~(ctrl_mask >> 4);
	// top 4 bits are a mask of which one's we're setting.
	// So we want (P4IN & ~(ctrl_mask>>4)) | ((ctrl_mask & 0xF) & (ctrl_mask>>4)
	// Get the old settings.
	tmp2 = P4OUT;
	// Mask off the bits that we're now controlling.
	tmp2 = tmp2 & tmp;
	// Mask off the bits we're NOT controlling, in case user was a jerk.
	ctrl_mask = (ctrl_mask) & ~tmp;
	tmp2 |= ctrl_mask;
	P4OUT = tmp2;
	uart0.tx('<');
	uart0.tx('c');
	uart0.tx(P4OUT & 0x0F);
	uart0.tx('>');
}

void vinmon_wakeup() {
	// Lower CLK (raise it here)
	// assume software waits some large time
	P1OUT |= BIT4;
}

void vinmon_setup() {
	// We're inverted, so keep CLK low.
	P1OUT &= ~BIT4;
	P1DIR |= BIT4;
}

uint16_t vinmon_read() {
	int i;
	uint16_t tmp;
	tmp = 0;
	for (i=0;i<16;i++) {
		P1OUT &= BIT4;
		__delay_cycles(500);
		tmp = tmp << 1;
		if (!(P1IN & BIT3))
			tmp |= 0x1;
		P1OUT |= BIT4;
		__delay_cycles(500);
	}
	// leave SCLK high
	P1OUT &= ~BIT4;
	return tmp;
}

void i2c_setup() {
	UCB0CTL1 = UCSWRST;
	UCB0CTL1 = UCSSEL_2 + UCTR;
	UCB0BR0 = 10;
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;
	UCB0I2CSA = 0x4C;
	UCB0CTL1 &= ~UCSWRST;
}

void adc_setup() {
	// The input impedances are all 90.9k, so the sampling time needs
	// to be around 150 us. We can get there by making the sampling
	// time 64 clocks, and then dividing the input clock by 3.
	ADC10CTL0 = ADC10SHT_3 + ADC10SR + REFBURST + REFON + REF2_5V + SREF_1;
	ADC10CTL1 = ADC10DIV_2 + ADC10SSEL_3;
	ADC10AE0 = BIT0 + BIT1 + BIT3 + BIT6;
	ADC10AE1 = BIT6;
}

uint16_t read_onboard(uint8_t idx) {
	ADC10CTL0 &= ~ENC;
	ADC10CTL1 &= ~INCH_15;
	ADC10CTL1 |= adc_map[idx];
	ADC10CTL0 |= ENC + ADC10SC;
	while (ADC10CTL1 & ADC10BUSY);
	return ADC10MEM;
}

uint16_t read_tmp422(uint8_t idx) {
	uint16_t tmp;
	while (UCB0CTL1 & UCTXSTP);
	// set pointer.
	UCB0CTL1 |= UCTXSTT;
	while (!(IFG2 & UCB0TXIFG));
	UCB0TXBUF = idx;
	while (UCB0CTL1 & UCTXSTT);
	if (UCB0STAT & UCNACKIFG) {
		UCB0CTL1 |= UCTXSTP;
		return 0xFFFF;
	}
	while (!(UCB0CTL1 & UCB0TXIFG));
	UCB0CTL1 |= UCTXSTP;
	while (UCB0CTL1 & UCTXSTP);
	UCB0CTL1 &= ~UCTR;
	UCB0CTL1 |= UCTXSTT;
	while (UCB0CTL1 & UCTXSTT);
	UCB0CTL1 |= UCTXSTP;
	if (UCB0STAT & UCNACKIFG) {
		return 0xFFFF;
	}
	while (!(UCB0CTL1 & UCB0RXIFG));
	tmp = UCB0RXBUF;
	return;
}

void handle_sensors(uint8_t cmd, uint8_t arg) {
	bool readable = (cmd == 'H');
	// rebase to readable characters.
	if (arg >= 'A') arg -= ('A' - '9' + 1);
	uint8_t onboard = (arg & 0x7);
	uint8_t temps = (arg>>3) & 0x3;
	bool vin_wakeup = ((arg & 0x60) == 0x40);
	bool vin_mon = ((arg&0x20) != 0);
	uint8_t i;
	uint16_t val;

	if (vin_wakeup) vinmon_wakeup();
	uart0.tx('<');
	uart0.tx(cmd+0x20);
	for (i=0;i<MAX_ADC;i++) {
		if (onboard == i ||
			onboard	== MAX_ADC) {
			val = read_onboard(i);
			if (readable) {
				puth((val & 0xF00)>>8);
				puth((val & 0x0F0)>>4);
				puth((val & 0x00F));
			} else {
				uart0.tx((val&0xF00)>>8);
				uart0.tx(val&0xFF);
			}
		}
	}
	for (i=0;i<MAX_TEMP;i++) {
		if (temps == i ||
			temps == MAX_TEMP) {
			val = read_tmp422(i);
			if (readable) {
				puth((val & 0xF00)>>8);
				puth((val & 0x0F0)>>4);
				puth((val & 0x00F));
			} else {
				uart0.tx((val & 0xF00)>>8);
				uart0.tx(val & 0xFF);
			}
		}
	}
	if (vin_mon) {
		val = vinmon_read();
		if (readable) {
			puth((val & 0xF000)>>12);
			puth((val & 0xF00)>>8);
			puth((val & 0x0F0)>>4);
			puth((val & 0x00F));
		} else {
			uart0.tx((val>>8) & 0xFF);
			uart0.tx(val & 0xFF);
		}
	}
	uart0.tx('>');
}

void handle_command() {
	switch(Ser0_command) {
	case 'H':
	case 'D':
		handle_sensors(Ser0_command, Ser0_argument);
		break;
	case 'C':
		handle_controls(Ser0_argument);
		break;
	case 'R':
		Ser0_argument &= 0x3F;
		uart0.tx('<');
		uart0.tx('r');
		uart0.tx(device_info_buffer[Ser0_argument]);
		uart0.tx('>');
		break;
	case 'P':
		if (!(Ser0_argument & 0xC0)) {
			device_info_wr_ptr = Ser0_argument;
		}
		uart0.tx('<');
		uart0.tx('p');
		uart0.tx(device_info_wr_ptr);
		uart0.tx('>');
		break;
	case 'W':
		device_info_buffer[device_info_wr_ptr] = Ser0_argument;
		device_info_wr_ptr++;
		device_info_wr_ptr &= 0x3F;
		uart0.tx('<');
		uart0.tx('w');
		uart0.tx(Ser0_argument);
		uart0.tx('>');
		break;
	case 'F':
		break;
	default:
		break;
	}
}

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    if (IFG1 & BIT2) {
    	// Power on. DEVICE_INFO_KEY programmed means that we're in normal
    	// operations, so pull P4OUT from device_info[0]. It should be 0.
    	if (device_info[DEVICE_INFO_SIZE-1] == DEVICE_INFO_KEY) {
    		P4OUT = device_info[0];
    	} else {
    		P4OUT = 0x0F;
    	}
    	P4DIR = 0x0F;
    }
    // If no power on, we don't touch P4OUT.
    BCSCTL1 = XT2OFF | CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	P1OUT = BIT2;
	P1DIR = BIT2;
	P3OUT = BIT1 + BIT2;
	P3REN = BIT1 + BIT2;
	P3SEL = BIT1 + BIT2 + BIT4 + BIT5;

	ADC10AE0 = BIT0 + BIT1 + BIT3 + BIT6;
	ADC10AE1 = BIT6;
	adc_setup();
	i2c_setup();
	vinmon_setup();
    components_init();
    while (1) {
    	components_loop();
    }
}
