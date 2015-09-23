//
//  pia.h
//  apple1
//
//  Created by Daniel Loffgren on 9/12/15.
//  Copyright (c) 2015 Daniel Loffgren. All rights reserved.
//

#ifndef __apple1__pia__
#define __apple1__pia__

#include <v6502/mem.h>
#include <curses.h>

#define A1PIA_KEYBOARD_INPUT_REGISTER	0xD010
#define A1PIA_KEYBOARD_READY_REGISTER	0xD011
#define A1PIA_VIDEO_OUTPUT_REGISTER		0xD012
#define A1PIA_VIDEO_ATTR_REGISTER		0xD013

typedef struct {
	/** @brief Curses output object */
	WINDOW *screen;
	/** @brief Hardwired memory used to trap video activity and report keyboard input */
	v6502_memory *memory;
	char buf;
	int signalled;
	int suspended;
} a1pia;

a1pia *pia_create(v6502_memory *mem);
void pia_destroy(a1pia *pia);

void pia_start(a1pia *pia);
void pia_stop(a1pia *pia);

#endif /* defined(__apple1__pia__) */
