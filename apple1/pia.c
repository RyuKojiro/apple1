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

void pia_map(v6502_memory *mem) {
	v6502_map(mem, A1PIA_KEYBOARD_INPUT, 1, FIXME_I_SHOULDNT_BE_NULL, NULL, NULL);
	v6502_map(mem, A1PIA_KEYBOARD_CRLF_REG, 1, FIXME_I_SHOULDNT_BE_NULL, NULL, NULL);
	v6502_map(mem, A1PIA_VIDEO_OUTPUT, 1, FIXME_I_SHOULDNT_BE_NULL, videoWriteCharCallback, NULL);
	v6502_map(mem, A1PIA_VIDEO_CRLF_REG, 1, FIXME_I_SHOULDNT_BE_NULL, videoWriteNewlineCallback, NULL);
}