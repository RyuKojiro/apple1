//
//  pia.h
//  apple1
//
//  Created by Daniel Loffgren on 9/12/15.
//  Copyright (c) 2015 Daniel Loffgren. All rights reserved.
//

#ifndef __apple1__pia__
#define __apple1__pia__

#include <v6502/cpu.h>
#include <curses.h>

#define A1PIA_KEYBOARD_INPUT_REGISTER	0xD010
#define A1PIA_KEYBOARD_READY_REGISTER	0xD011
#define A1PIA_VIDEO_OUTPUT_REGISTER		0xD012
#define A1PIA_VIDEO_ATTR_REGISTER		0xD013

typedef struct {
	/** @brief Curses output object */
	WINDOW *screen;
	/** @brief Hardwired cpu used to trap video activity and report
	 * keyboard input back via memory mapped registers. */
	v6502_cpu *cpu;
	char buf;
	int signalled;
	int suspended;
	char history[2048];
	int hist_pos;
} a1pia;

a1pia *pia_create(v6502_cpu *cpu);
void pia_destroy(a1pia *pia);

void pia_start(a1pia *pia, int continuous);
void pia_stop(a1pia *pia);

/* The freeze format is as follows:
 * 0x00000-0x0FFFF: The contents of memory
 * 0x10001-0x10002: CPU Program Counter
 *         0x10003: CPU Accumulator
 *         0x10004: CPU X Register
 *         0x10005: CPU Y Register
 *         0x10006: CPU Status Register
 * 0x10007-0x10807: Video Buffer History
 *
 * The video buffer history is a 2 kilobyte ring buffer that is preserved
 * by the PIA. On load, it is replayed, which should restore the video
 * output to a pretty close approximation, depending on terminal size and
 * history.
 */
void saveFreeze(a1pia *pia, const char *fname);
void loadFreeze(a1pia *pia, const char *fname);

#endif /* defined(__apple1__pia__) */
