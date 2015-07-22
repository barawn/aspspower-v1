/*
 * components.h
 *
 * In the lwevents framework, the components.h/components.cpp file contains the instantiation and global names of all of the
 * main components.
 *
 * Generally if a function expects there to be something in the system (like an event queue, or a UART) they'll use these names.
 *
 *  Created on: Jul 1, 2015
 *      Author: barawn
 */

#ifndef COMPONENTS_H_
#define COMPONENTS_H_

#include "usci_ser0.h"
#include "lwevent.h"

typedef USCI_Ser0 Ser0;

extern Ser0 uart0;
extern isr_lwevent_queue queue0;
extern lwevent_queue queue1;

void components_init();
void components_loop();

#endif /* COMPONENTS_H_ */
