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
#include <unistd.h>

#define FIXME_I_SHOULDNT_BE_NULL NULL
#define KEYBOARD_READY 0xFF // This just needs to meet the requirements of being a negative number in the eyes of the 6502
#define KEYBOARD_NOTREADY 0x00
#define ANSI_BGCOLOR_GREEN   "\x1b[42;1m"

void saveFreeze(v6502_memory *mem, const char *fname) {
	FILE *f = fopen(fname, "w");
	if (!f) {
		endwin();
		fprintf(stderr, "Couldn't open %s for writing\n", fname);
	}
	
	for (uint16_t offset = 0; offset < v6502_memoryStartCeiling; offset++) {
		uint8_t byte = v6502_read(mem, offset, NO);
		fwrite(&byte, sizeof(uint8_t), 1, f);
	}
	
	fclose(f);
}

char asciiCharFromCursesKey(int key) {
	return (char)key;
}

char asciiCharFromA1Char(uint8_t c) {
	return (char)c & ~0x80;
}

uint8_t a1CharFromAsciiChar(char c) {
	if (c >= 'a' && c <= 'z') {
		c -= 0x20;
	}
	
	return (char)c | 0x80;
}

void videoWriteCharCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, a1pia *context) {
	if (value) {
		char c = asciiCharFromA1Char(value);
		if (c == '\r') {
			fprintf(stdout, "\n");
		}
		if (c == 0x7f) {
			int y, x;
			getyx(context->screen, y, x);
			if (x < 0) {
				move(y, x-1);
			}
			delch();
			refresh();
		}
		else {
			fprintf(stdout, ANSI_COLOR_BRIGHT_GREEN "%c" ANSI_COLOR_RESET, c);
		}
		//memory->bytes[offset] = value;
		fflush(stdout);
	}
}

void videoWriteNewlineCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, a1pia *context) {
	fprintf(stdout, "\r\n");
	fflush(stdout);
}

uint8_t keyboardReadReadyCallback(struct _v6502_memory *memory, uint16_t offset, int trap, a1pia *context) {
	if (!trap) {
		return 0xbf;
	}
	
	if (context->buf) {
		return KEYBOARD_READY;
	}
	
	saveFreeze(memory, "freeze.ram");
	int c = getch();
	
	if (c != ERR) {
		context->buf = asciiCharFromCursesKey(c);
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
	printf(ANSI_BGCOLOR_GREEN);
	
	for (int x = 0; x < 80*25; x++) {
		printf(" ");
	}
	
	printf(ANSI_COLOR_RESET);
	fflush(stdout);
	usleep(100000);
}

a1pia *pia_create(v6502_memory *mem) {
	a1pia *pia = malloc(sizeof(a1pia));
	pia->memory = mem;
	pia->screen = initscr();
	//nodelay(stdscr, true);
	crmode();
	noecho();
	nonl();
	
	v6502_map(mem, A1PIA_KEYBOARD_INPUT, 1, (v6502_readFunction *)keyboardReadCharacterCallback, NULL, pia);
	v6502_map(mem, A1PIA_KEYBOARD_CRLF_REG, 1, (v6502_readFunction *)keyboardReadReadyCallback, NULL, pia);
	v6502_map(mem, A1PIA_VIDEO_OUTPUT, 1, FIXME_I_SHOULDNT_BE_NULL, (v6502_writeFunction *)videoWriteCharCallback, pia);
	v6502_map(mem, A1PIA_VIDEO_CRLF_REG, 1, FIXME_I_SHOULDNT_BE_NULL, (v6502_writeFunction *)videoWriteNewlineCallback, pia);

	_doCoolVideoStart(pia);
	return pia;
}

void pia_destroy(a1pia *pia) {
	endwin();
	free(pia);
}
