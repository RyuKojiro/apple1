//
//  pia.c
//  apple1
//
//  Created by Daniel Loffgren on 9/12/15.
//  Copyright (c) 2015 Daniel Loffgren. All rights reserved.
//

#include "pia.h"
#include <stdio.h>
#include <as6502/color.h>
#include <stdlib.h>

#define FIXME_I_SHOULDNT_BE_NULL NULL

char asciiCharFromA1Char(uint8_t c) {
	switch (c) {
		case 0xDC: return '\\';
		case 0x8D: return '\n';
	}
	return (char)c;
}

void videoWriteCharCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, void *context) {
	if (value) {
		fprintf(stdout, ANSI_COLOR_BRIGHT_GREEN "%c" ANSI_COLOR_RESET, asciiCharFromA1Char(value));
		//fprintf(stderr, "I was asked to print (0x%02x)\n", value);
		//memory->bytes[offset] = value;
		fflush(stdout);
	}
}

void videoWriteNewlineCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, void *context) {
	fprintf(stdout, ANSI_COLOR_BRIGHT_GREEN "\n" ANSI_COLOR_RESET);
	fflush(stdout);
}

uint8_t keyboardReadNewlineCallback(struct _v6502_memory *memory, uint16_t offset, int trap, void *context) {
	// FIXME: this is just to satiate the woz monitor until real input is hooked up
	return 1;
}

static void _doCoolVideoStart(a1pia *pia) {
	
}

a1pia *pia_create(v6502_memory *mem) {
	a1pia *pia = malloc(sizeof(a1pia));
	pia->memory = mem;
	pia->screen = initscr();

	v6502_map(mem, A1PIA_KEYBOARD_INPUT, 1, FIXME_I_SHOULDNT_BE_NULL, NULL, pia);
	v6502_map(mem, A1PIA_KEYBOARD_CRLF_REG, 1, keyboardReadNewlineCallback, NULL, pia);
	v6502_map(mem, A1PIA_VIDEO_OUTPUT, 1, FIXME_I_SHOULDNT_BE_NULL, videoWriteCharCallback, pia);
	v6502_map(mem, A1PIA_VIDEO_CRLF_REG, 1, FIXME_I_SHOULDNT_BE_NULL, videoWriteNewlineCallback, pia);

	_doCoolVideoStart(pia);
	return pia;
}

void pia_destroy(a1pia *pia) {
	endwin();
	free(pia);
}
