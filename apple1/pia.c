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
#define KEYBOARD_READY 0xFF // This just needs to meet the requirements of being a negative number in the eyes of the 6502
#define KEYBOARD_NOTREADY 0x00

char asciiCharFromA1Char(uint8_t c) {
	switch (c) {
		case 0xDC: return '\\';
		case 0x8D: return '\n';
	}
	return (char)c & ~0x20;
}

uint8_t a1CharFromAsciiChar(char c) {
	switch (c) {
		case '\\': return 0xDC;
		case '\n': return 0x8D;
	}
	return (char)c;
}

void videoWriteCharCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, void *context) {
	if (value) {
		fprintf(stdout, ANSI_COLOR_BRIGHT_GREEN "%c" ANSI_COLOR_RESET, asciiCharFromA1Char(value));
//		fprintf(stderr, "I was asked to print (0x%02x)\n", value);
		//memory->bytes[offset] = value;
		fflush(stdout);
	}
}

void videoWriteNewlineCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, void *context) {
	fprintf(stdout, ANSI_COLOR_BRIGHT_GREEN "\r\n" ANSI_COLOR_RESET);
	fflush(stdout);
}

uint8_t keyboardReadReadyCallback(struct _v6502_memory *memory, uint16_t offset, int trap, a1pia *context) {
	if (context->buf) {
		return KEYBOARD_READY;
	}
	
	int c = getch();
	
	if (c != ERR) {
		context->buf = c;
		return KEYBOARD_READY;
	}
	
	return KEYBOARD_NOTREADY;
}

uint8_t keyboardReadCharacterCallback(struct _v6502_memory *memory, uint16_t offset, int trap, a1pia *context) {
	if (context->buf) {
		uint8_t a = a1CharFromAsciiChar(context->buf);
		context->buf = '\0';
		return a;
	}
	
	return 0;
}

static void _doCoolVideoStart(a1pia *pia) {
	
}

a1pia *pia_create(v6502_memory *mem) {
	a1pia *pia = malloc(sizeof(a1pia));
	pia->memory = mem;
	pia->screen = initscr();
	//nodelay(stdscr, true);
	crmode();
	noecho();
	
	v6502_map(mem, A1PIA_KEYBOARD_INPUT, 1, (v6502_readFunction *)keyboardReadCharacterCallback, NULL, pia);
	v6502_map(mem, A1PIA_KEYBOARD_CRLF_REG, 1, (v6502_readFunction *)keyboardReadReadyCallback, NULL, pia);
	v6502_map(mem, A1PIA_VIDEO_OUTPUT, 1, FIXME_I_SHOULDNT_BE_NULL, videoWriteCharCallback, pia);
	v6502_map(mem, A1PIA_VIDEO_CRLF_REG, 1, FIXME_I_SHOULDNT_BE_NULL, videoWriteNewlineCallback, pia);

	_doCoolVideoStart(pia);
	return pia;
}

void pia_destroy(a1pia *pia) {
	endwin();
	free(pia);
}
