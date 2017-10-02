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
#include <assert.h>

#define KEYBOARD_READY     0xFF // This just needs to meet the requirements of being a negative number in the eyes of the 6502
#define KEYBOARD_NOTREADY  0x00
#define ANSI_BGCOLOR_GREEN "\x1b[42;1m"
#define CURSES_BACKSPACE   0x7F
#define A1_BACKSPACE       0xDF

void saveFreeze(a1pia *pia, const char *fname) {
	FILE *f = fopen(fname, "w");
	if (!f) {
		endwin();
		fprintf(stderr, "Couldn't open %s for writing\n", fname);
	}

	// See header for a format description.
	for (uint16_t offset = 0; offset < v6502_memoryStartCeiling; offset++) {
		uint8_t byte = v6502_read(pia->cpu->memory, offset, NO);
		fwrite(&byte, sizeof(uint8_t), 1, f);
	}

	uint8_t pcHigh = pia->cpu->pc >> 8;
	uint8_t pcLow = pia->cpu->pc & 0xFF;
	fwrite(&pcLow,        1, 1, f); // Low
	fwrite(&pcHigh,       1, 1, f); // High
	fwrite(&pia->cpu->ac, 1, 1, f);
	fwrite(&pia->cpu->x,  1, 1, f);
	fwrite(&pia->cpu->y,  1, 1, f);
	fwrite(&pia->cpu->sr, 1, 1, f);
	fwrite(&pia->cpu->sp, 1, 1, f);

	fclose(f);
}

void loadFreeze(a1pia *pia, const char *fname) {
	FILE *f = fopen(fname, "r");
	if (!f) {
		endwin();
		fprintf(stderr, "Couldn't open %s for reading\n", fname);
	}

	// See header for a format description.
	for (uint16_t offset = 0; offset < v6502_memoryStartCeiling; offset++) {
		uint8_t byte;
		fread(&byte, sizeof(uint8_t), 1, f);
		pia->cpu->memory->bytes[offset] = byte;
	}

	uint8_t pcLow, pcHigh;
	fread(&pcLow,        1, 1, f); // Low
	fread(&pcHigh,       1, 1, f); // High
	pia->cpu->pc = pcLow | (pcHigh << 8);

	fread(&pia->cpu->ac, 1, 1, f);
	fread(&pia->cpu->x,  1, 1, f);
	fread(&pia->cpu->y,  1, 1, f);
	fread(&pia->cpu->sr, 1, 1, f);
	fread(&pia->cpu->sp, 1, 1, f);

	fclose(f);
}

unsigned char asciiCharFromCursesKey(int key) {
	return (unsigned char)key;
}

unsigned char asciiCharFromA1Char(uint8_t c) {
	if (c == A1_BACKSPACE) {
		return CURSES_BACKSPACE;
	}

	return (unsigned char)c & ~0x80;
}

uint8_t a1CharFromAsciiChar(unsigned char c) {
	if (c == CURSES_BACKSPACE) {
		return A1_BACKSPACE;
	}

	if (c >= 'a' && c <= 'z') {
		c -= 0x20;
	}
	
	return (char)c | 0x80;
}

void videoWriteCharCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, a1pia *context) {
	if (value) {
		unsigned char c = asciiCharFromA1Char(value);
		if (c == '\r') {
			waddch(context->screen, '\n');
		}
		if (c == CURSES_BACKSPACE) {
			int y, x;
			getyx(context->screen, y, x);
			if (x > 0) {
				move(y, x-1);
			}
			delch();
			refresh();
		}
		else {
//			printf(ANSI_COLOR_BRIGHT_GREEN);
			waddch(context->screen, c);
//			printf(ANSI_COLOR_RESET);
		}
		//memory->bytes[offset] = value;
	}
}

void videoWriteNewlineCallback(struct _v6502_memory *memory, uint16_t offset, uint8_t value, a1pia *context) {
//	fprintf(stdout, "\r\n");
//	fflush(stdout);
}

uint8_t keyboardReadReadyCallback(struct _v6502_memory *memory, uint16_t offset, int trap, a1pia *context) {
	if (!trap) {
		return 0xbf;
	}
	
	if (context->buf) {
		return KEYBOARD_READY;
	}
	
	if (context->suspended) {
		printf("Keyboard readiness register ($D011) trap read.\n");
		printf("Press a key for input to keyboard register ($D010): ");
		fflush(stdout);
		crmode();
	}
	else {
		int x, y;
		getyx(context->screen, y, x);
		wmove(context->screen, 0, getmaxx(context->screen) - 1);
		attroff(A_REVERSE);
		wprintw(context->screen, " ");
		wmove(context->screen, y, x);
	}

	int c = wgetch(context->screen);
	if (context->suspended) {
		printf("%c\r\n", c);
	}
	else {
		int x, y;
		getyx(context->screen, y, x);
		wmove(context->screen, 0, getmaxx(context->screen) - 1);
		attron(A_REVERSE);
		wprintw(context->screen, "!");
		attroff(A_REVERSE);
		wmove(context->screen, y, x);
	}
	
	if (c == '`') {
		context->signalled++;
		return KEYBOARD_NOTREADY;
	}
	
	if (c != ERR) {
		context->buf = asciiCharFromCursesKey(c);
		return KEYBOARD_READY;
	}
	
	return KEYBOARD_NOTREADY;
}

uint8_t keyboardReadCharacterCallback(struct _v6502_memory *memory, uint16_t offset, int trap, a1pia *context) {
	// This means we are in the debugger, but the CPU actually stepped over an instruction (rather than a tool peeking at memory)
	if (context->suspended && trap) {
		printf("Keyboard register ($D010) read.\n");
		printf("Buffered character was #$%02x, now empty.\n", a1CharFromAsciiChar(context->buf));
		fflush(stdout);
		crmode();
	}

	if (context->buf) {
		uint8_t a = a1CharFromAsciiChar(context->buf);
		if (trap) {
			context->buf = '\0';
		}
		return a;
	}
	
	return 0;
}

a1pia *pia_create(v6502_cpu *cpu) {
	a1pia *pia = malloc(sizeof(a1pia));
	pia->cpu = cpu;
	pia->screen = NULL;
	pia->buf = '\0';

	assert(v6502_map(cpu->memory, A1PIA_KEYBOARD_INPUT_REGISTER, 1, (v6502_readFunction *)keyboardReadCharacterCallback, NULL, pia));
	assert(v6502_map(cpu->memory, A1PIA_KEYBOARD_READY_REGISTER, 1, (v6502_readFunction *)keyboardReadReadyCallback, NULL, pia));
	assert(v6502_map(cpu->memory, A1PIA_VIDEO_OUTPUT_REGISTER, 1, NULL, (v6502_writeFunction *)videoWriteCharCallback, pia));
	assert(v6502_map(cpu->memory, A1PIA_VIDEO_ATTR_REGISTER, 1, NULL, (v6502_writeFunction *)videoWriteNewlineCallback, pia));

	return pia;
}

void pia_destroy(a1pia *pia) {
	endwin();
	free(pia);
}

void pia_start(a1pia *pia, int continuous) {
	if (!pia->screen) {
		pia->screen = initscr();
	}
	nodelay(stdscr, continuous);
	crmode();
	noecho();
	nonl();
	scrollok(pia->screen, YES);
	pia->signalled = 0;
	pia->suspended = 0;
	wrefresh(pia->screen);
	start_color();
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	attron(COLOR_PAIR(1) | A_BOLD);
}

void pia_stop(a1pia *pia) {
	pia->suspended = 1;
	pia->screen = NULL;
	endwin();
}
