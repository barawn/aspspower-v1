/*
 * lwevent.h: The base of the lightweight event library.
 *
 * Lightweight events are ways to write microcontroller modules which can be written in an event-based
 * format, rather than a polling format, in probably close to the fastest way possible.
 *
 * The lwevent setup essentially creates a last-in, first-out linked list of callbacks which are
 * statically created, so there's no overhead of adding things into a FIFO, checking if there's
 * enough space, etc. Adding a callback to the list is nothing more than:
 * 1) test 'next' pointer; if non-zero, return (event already posted)
 * 2) move 'head' to 'next' pointer
 * 3) move event to 'head'
 * On most architectures this is a very, very fast operation.
 *
 * The lwevent queue is implemented using CRTP, with lwevent_queue_base as the base class.
 * There are two basic implementations included: the lwevent_queue implementation, which
 * is for events which can only be posted outside of an ISR - this avoids the overhead
 * of disabling/reenabling interrupts all the time.
 *
 * The isr_lwevent_queue implementation uses a global register, at least for as long as
 * TI allows us to reserve one (--global-register=r4).
 *
 *  Created on: Jun 25, 2015
 *      Author: barawn
 */

#ifndef LWEVENT_H_
#define LWEVENT_H_

#include <msp430.h>

class lwevent;

typedef void (*lwevent_handler_t)(lwevent *);

class lwevent {
public:
	enum {
			LWEVENT_WAITING = 0x0000,
			LWEVENT_TAIL = 0xFFFF,
			LWEVENT_DISABLED = 0x0001
	};
	lwevent_handler_t handler;
	lwevent *next;
	lwevent(lwevent_handler_t init_handler, lwevent *enabled) : handler(init_handler), next(enabled) {}
	void clear() {
		next = (lwevent *) lwevent::LWEVENT_WAITING;
	}
	void disable() {
		next = (lwevent *) lwevent::LWEVENT_DISABLED;
	}
	bool is_disabled() {
		return (next == (lwevent *) lwevent::LWEVENT_DISABLED);
	}
};

template<class T>
class lwevent_store_base {
protected:
	lwevent *head;
public:
	lwevent_store_base() : head((lwevent *) lwevent::LWEVENT_TAIL) {}
	bool empty() {
		return (head == (lwevent *) lwevent::LWEVENT_TAIL);
	}
	void store(lwevent *p) {
		static_cast<T*>(this)->store_impl(p);
	}
	lwevent *get_first() {
		return static_cast<T*>(this)->get_first_impl();
	}
	lwevent *get_last() {
		return static_cast<T*>(this)->get_last_impl();
	}
	lwevent *pop() {
		return static_cast<T*>(this)->pop_impl();
	}
};

/** \brief A last-in, first-out lwevent store. A bit faster on insertion/removal than FIFO. Slower on a full-insert into a queue.
 *
 */
class lwevent_store_lifo : public lwevent_store_base<lwevent_store_lifo> {
public:
	lwevent_store_lifo() {}
	void store_impl(lwevent *p) {
		p->next = head;
		head = p;
	}
	lwevent *get_first_impl() {
		return head;
	}
	lwevent *get_last_impl() {
		lwevent *p;
		if (head == (lwevent *) lwevent::LWEVENT_TAIL) return head;
		for (p=head; p->next != (lwevent *) lwevent::LWEVENT_TAIL; p=p->next) ;
		return p;
	}
	lwevent *pop_impl() {
		lwevent *p;
		p = head;
		if (p != (lwevent *) lwevent::LWEVENT_TAIL) head = p->next;
		return p;
	}
};

/** \brief A first-in, first-out lwevent store.
 *
 */
class lwevent_store_fifo : public lwevent_store_base<lwevent_store_fifo> {
private:
	lwevent *tail;
public:
	lwevent_store_fifo() : tail((lwevent *) lwevent::LWEVENT_TAIL) {}
	void store_impl(lwevent *p) {
		if (tail == (lwevent *) lwevent::LWEVENT_TAIL) {
			p->next = head;
			head = p;
			tail = p;
		} else {
			tail->next = p;
			tail = p;
		}
	}
	lwevent *pop_impl() {
		lwevent *p;
		p = head;
		if (p != (lwevent *) lwevent::LWEVENT_TAIL) {
			head = p->next;
			if (tail == p) tail = head;
		}
		return p;
	}
	lwevent *get_first_impl() {
		return head;
	}
	lwevent *get_last_impl() {
		return tail;
	}
};

template<class T>
class lwevent_queue_base {
public:
	void init() {
		static_cast<T*>(this)->init_impl();
	}
	void process() {
		lwevent *p = static_cast<T*>(this)->detach_head();
		while (p != (lwevent *) lwevent::LWEVENT_TAIL) {
			lwevent *pnext = p->next;
			p->handler(p);
			p = pnext;
		}
	}
	bool sleep_check() {
		return static_cast<T*>(this)->sleep_check_impl();
	}
	bool post(lwevent *p) {
		T::post_begin();
		if (p->next) return false;
		p->next = static_cast<T*>(this)->get_head();
		static_cast<T*>(this)->set_head(p);
		T::post_end();
		return true;
	}
	void post_without_check(lwevent *p) {
		T::post_begin();
		p->next = static_cast<T*>(this)->get_head();
		static_cast<T*>(this)->set_head(p);
		T::post_end();
	}
	/** \brief Insert an already linked list of lwevents.
	 * 2 notes:
	 * - The "first" lwevent here gets placed first. The "last" gets placed last, but before all old lwevents.
	 * - This function says "_without_check" but obviously this is done because the lwevents were in a store somewhere,
	 *   so they're obviously not posted.
	 * */
	void post_multiple_without_check(lwevent *first, lwevent *last) {
		T::post_begin();
		last->next = static_cast<T*>(this)->get_head();
		static_cast<T*>(this)->set_head(first);
		T::post_end();
	}
	/** \brief Insert everything in a lwevent store.
	 *
	 * Uses post_multiple_without_check.
	 *
	 */
	template<typename T2>
	void post_store(T2& store) {
		post_multiple_without_check(store.get_first(), store.get_last());
	}
};

/** \brief An lwevent_queue for events that cannot be posted from ISRs.
 *
 */
class lwevent_queue : public lwevent_queue_base<lwevent_queue> {
private:
	lwevent *head;
public:
	lwevent_queue() : head((lwevent *) lwevent::LWEVENT_TAIL) {}
	void init_impl() { }
	lwevent *detach_head() {
		lwevent *p;
		p = head;
		head = (lwevent *) lwevent::LWEVENT_TAIL;
		return p;
	}
	lwevent *get_head() {
		return head;
	}
	void set_head(lwevent *p) {
		head = p;
	}
	static void post_begin() {}
	static void post_end() {}
	bool sleep_check_impl() {
		return (head == (lwevent *) lwevent::LWEVENT_TAIL);
	}
};

// lwevent queue for events which are called within an ISR.
// This queue uses a dedicated register for the lwevent queue head, which improves 2 things:
// 1) makes the ISR faster (12 cycles instead of 16 for a posted event with checking,
//    5 cycles instead of 9 cycles for a posted event without checking).
// 2) reduces the time that interrupts are disabled during the queue processing
//    (3 cycles instead of 8).
class isr_lwevent_queue : public lwevent_queue_base<isr_lwevent_queue> {
public:
	isr_lwevent_queue() {}
	static void init_impl() {
		asm("	mov.w	#0xFFFF, r4");
	}
	static lwevent *detach_head() {
		lwevent *p;
		__disable_interrupt();
		p = (lwevent *) __get_R4_register();
		asm("\t mov.w #(0xFFFF), r4");
		__enable_interrupt();
		return p;
	}
	static lwevent *get_head() {
		lwevent *p;
		p = (lwevent *) __get_R4_register();
		return p;
	}
	static void set_head(lwevent *p) {
		__set_R4_register((unsigned short) p);
	}
	static void post_from_isr_without_check(lwevent *p) {
		p->next = (lwevent *) __get_R4_register();
		__set_R4_register((unsigned short) p);
	}
	static bool post_from_isr(lwevent *p) {
		if (p->next) return false;
		p->next = (lwevent *) __get_R4_register();
		__set_R4_register((unsigned short) p);
		return true;
	}
	static void post_begin() {
		__disable_interrupt();
	}
	static void post_end() {
		__enable_interrupt();
	}
	// NOTE: you shouldn't call sleep_check on the ISR queue. Call sleep() instead, it's more efficient,
	// because it can just do the check on the r4 register directly (stupid compiler!)
	// Note also that sleep_check for the ISR queue *leaves interrupts disabled* if it returns true.
	// This is intentional, since it wouldn't make sense for an ISR queue to say it CAN sleep with interrupts
	// still enabled, since a lwevent could post right after reenabling interrupts.
	static bool sleep_check_impl() {
		lwevent *p;
		__disable_interrupt();
		p = (lwevent *) __get_R4_register();
		if (p != (lwevent *) lwevent::LWEVENT_TAIL) {
			__enable_interrupt();
			return false;
		}
		return true;
	}
	static void sleep() {
		__disable_interrupt();
		asm("\t cmp.w	#0xFFFF, r4\n"
			"\t jne 	isr_lwevent_queue_sleep_wakeup?\n"
			"\t bis.w	#0x18, SR\n"
			"\t nop\n"
			"isr_lwevent_queue_sleep_wakeup?:\n");
		__enable_interrupt();
	}
};

#endif /* LWEVENT_H_ */
