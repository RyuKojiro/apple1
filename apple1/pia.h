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

#define A1PIA_KEYBOARD_INPUT	0xD010
#define A1PIA_KEYBOARD_CRLF_REG	0xD011
#define A1PIA_VIDEO_OUTPUT		0xD012
#define A1PIA_VIDEO_CRLF_REG	0xD013

typedef struct {
	/** @brief Curses output object */
	WINDOW *screen;
	/** @brief Hardwired memory used to trap video activity and report keyboard input */
	v6502_memory *memory;
} a1pia;

a1pia *pia_create(v6502_memory *mem);
void pia_destroy(a1pia *pia);

#endif /* defined(__apple1__pia__) */
