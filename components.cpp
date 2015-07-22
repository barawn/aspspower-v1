/*
 * components.cpp
 *
 *  Created on: Jul 1, 2015
 *      Author: barawn
 */

#include "components.h"

Ser0 uart0;
isr_lwevent_queue queue0;
lwevent_queue queue1;

void components_init() {
	queue0.init();
	queue1.init();
	uart0.init();
}

void components_loop() {
	queue0.process();
	queue1.process();
	if (!uart0.sleep_check()) return;
	if (!queue1.sleep_check()) return;
	queue0.sleep();
}
