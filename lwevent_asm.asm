; lwevent macro library.
;
; These macros assist in posting lwevents from an ISR.
;
; The common ones are isr_lwevent_post, isr_lwevent_post_nocheck,
; and isr_lwevent_post_wakeup.
;
; Functionally the macros are a call to isr_lwevent_post_nocheck surrounded by
; a test/jump.
;
; isr_lwevent_post LWEVENT
;        Normal macro for posting an lwevent in an ISR. Checks if lwevent has been posted, and adds to ISR queue if it has.
;        Sets Z if the lwevent was actually posted.
;        Clears Z if the lwevent was not posted.
;
; isr_lwevent_post_nocheck LWEVENT
;        Post lwevent without checking. Can be done if it's known that the ISR *cannot* post it twice;
;        for instance, if the ISR disables its interrupt and only the lwevent handler can reenable it.
;        Does not touch flags.
;
; isr_lwevent_post_wakeup LWEVENT STACK_OFFSET_TO_SR
;        Same as isr_lwevent_post, but if lwevent posts, exits LPM.
;        The savings here is that the jump over the lwevent posting can also jump over the LPM exiting.
;        STACK_OFFSET_TO_SR is normally 0 if the stack is back at its starting position, but if the
;        assembly function has incremented it (using push) you need to increase it by the appropriate
;        amount (if, e.g., you've done "push r15", STACK_OFFSET_TO_SR would be 2, since r15 is 2 bytes).
;
; Specialty macros:
;
; These can be used (in combination with isr_lwevent_post_nocheck) to tweak flag settings a bit
; after the check. This is because the MSP430 has 3 ways to test zero:
; tst.w &value
; add.w #0, &value
; bit.w #0xFFFF, &value
;
; If the lwevent post is followed by a CLRC, for instance, the clrC macro can save that instruction.
; Note that this MAY or MAY NOT actually be helpful, since some MSP430 variants execute tst.w &value
; in 4 cycles, and some do it in 3 cycles. add.w is always 4 cycles. bit.w is identical to tst.w
;
; isr_lwevent_check_setC LWEVENT
; 		 Sets Z if a lwevent has not previously been posted and is enabled.
;        Always sets C. 3 or 4 cycles depending on variant.
; isr_lwevent_check_clrC LWEVENT
; 		 Sets Z if a lwevent has not previously been posted and is enabled.
;        Always clears C. 4 cycles.
; isr_lwevent_checkk_CnotZ LWEVENT
;        Sets Z if a lwevent has not previously been posted and is enabled.
;        C becomes not Z. 3 or 4 cycles depending on variant.

isr_lwevent_post_nocheck		.macro	LWEVENT
			mov.w	r4, &:LWEVENT:.next
			mov.w	#:LWEVENT:, r4
			.endm

isr_lwevent_post				.macro	LWEVENT
			tst.w	&:LWEVENT:.next						; 4 cycles 2 words
			jnz		isr_lwevent_post_done?					; 2 cycles 2 words
			isr_lwevent_post_nocheck	:LWEVENT:
isr_lwevent_post_done?:
			.endm

isr_lwevent_post_wakeup			.macro	LWEVENT, STACK_OFFSET_TO_SR
			tst.w	&:LWEVENT:.next
			jnz		isr_lwevent_post_wakeup_done?
			isr_lwevent_post_nocheck	:LWEVENT:
			bic		#(LPM3), :STACK_OFFSET_TO_SR:(SP)
isr_lwevent_post_wakeup_done?:
			.endm

isr_lwevent_check_setC			.macro	LWEVENT
			tst.w	&:LWEVENT:.next
			.endm

isr_lwevent_check_clrC			.macro	LWEVENT
			add.w	&:LWEVENT:.next
			.endm

isr_lwevent_check_CnotZ			.macro	LWEVENT
			bit.w	#(0xFFFF), &:LWEVENT:.next
			.endm
