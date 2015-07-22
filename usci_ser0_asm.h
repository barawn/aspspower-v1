/*
 * usci_ser0_asm.h
 *
 *  Created on: Jul 21, 2015
 *      Author: barawn
 */

#ifndef USCI_SER0_ASM_H_
#define USCI_SER0_ASM_H_

#include <stdint.h>
#include "usci_ser0.h"

//% \brief Assembly convenience enum for USCI Ser0.
//%
//% The MSP430 assembler's .cdecl directive isn't
//% great at handling C++ code - if you put these
//% as constants, it just skips over them. But!
//% If you define them as enums, then magically,
//% it supports them. So we just port over all of
//% the important constants.
//%
//% Note that if we included this in the C++ class,
//% then the symbols would be mangled, and we would
//% have to extract their actual names. Leaving them
//% outside like this eliminates the mangling,
//% and then we know what they look like.
enum USCI_Ser0_Config_enum {
	USCI_INDEX = USCI_Ser0_Config::USCI_INDEX,
	TX_BUF_SIZE = USCI_Ser0_Config::TX_BUF_SIZE,
	RX_BUF_SIZE = USCI_Ser0_Config::RX_BUF_SIZE
};





#endif /* USCI_SER0_ASM_H_ */
