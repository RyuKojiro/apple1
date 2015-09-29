//
//  main.c
//  apple1
//
//  Created by Daniel Loffgren on 9/12/15.
//  Copyright (c) 2015 Daniel Loffgren. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <histedit.h>
#include <signal.h>
#include <stdlib.h>

#include <dis6502/reverse.h>
#include <v6502/log.h>
#include <v6502/debugger.h>
#include <v6502/cpu.h>
#include <v6502/mem.h>
#include <as6502/error.h>

#include "pia.h"

#define ROM_START		0xF000
#define ROM_SIZE		0x00FF
#define RESET_VECTOR	0xFF00
#define DEBUGGER_MESSAGE	" [ Hit ` for debugger ] "

static v6502_cpu *cpu;
static a1pia *pia;
static int consoleMessageSeen;
static as6502_symbol_table *table;
static v6502_breakpoint_list *breakpoint_list;

static void fault(void *ctx, const char *e) {
	(*(int *)ctx)++;
}

//static uint8_t romMirrorCallback(struct _v6502_memory *memory, uint16_t offset, int trap, void *context) {
//	return memory->bytes[offset % ROM_SIZE];
//}

static void run(v6502_cpu *cpu) {
	int faulted = 0;
	cpu->fault_callback = fault;
	cpu->fault_context = &faulted;

	FILE *asmfile = fopen("runtime.s", "w");
	pia_start(pia);
	
	// This depends on the fact that pia_start will get curses up and going
	if (!consoleMessageSeen) {
		wmove(pia->screen, getmaxy(pia->screen) - 1, getmaxx(pia->screen) - sizeof(DEBUGGER_MESSAGE));
		attron(A_REVERSE);
		wprintw(pia->screen, DEBUGGER_MESSAGE);
		attroff(A_REVERSE);
		wmove(pia->screen, 0, 0);
		consoleMessageSeen++;
	}
	
	while (!faulted && !pia->signalled) {
		if (v6502_breakpointIsInList(breakpoint_list, cpu->pc)) {
			pia_stop(pia);
			fclose(asmfile);
			printf("Hit breakpoint at 0x%02x.\n", cpu->pc);
			return;
		}

		dis6502_printAnnotatedInstruction(asmfile, cpu, cpu->pc, table);
		v6502_step(cpu);
		v6502_printCpuState(asmfile, cpu);
	}
	pia_stop(pia);
	fclose(asmfile);
}

static const char *prompt() {
	static char prompt[10];
	snprintf(prompt, 10, "(0x%04x) ", cpu->pc);
	return prompt;
}

int main(int argc, const char * argv[])
{
	currentFileName = "apple1";

	cpu = v6502_createCPU();
	printf("Allocating 64k of memory...\n");
	cpu->memory = v6502_createMemory(v6502_memoryStartCeiling + 1);

	breakpoint_list = v6502_createBreakpointList();
	table = as6502_createSymbolTable();
	
	// Load Woz Monitor
	printf("Loading ROM...\n");
	v6502_loadFileAtAddress(cpu->memory, "apple1.rom", RESET_VECTOR);
	//v6502_map(cpu->memory, start, ROM_SIZE, romMirrorCallback, NULL, NULL);
	
	// Load debugger script
	int verbose = 0;
	FILE *file = fopen("apple1.dbg", "r");
	if (file) {
		printf("Loading debugger script...\n");
		v6502_runDebuggerScript(cpu, file, breakpoint_list, table, run, &verbose);
		fclose(file);
	}
	
	// Attach PIA
	printf("Initializing PIA...\n");
	pia = pia_create(cpu->memory);
	
	printf("Resetting CPU...\n");
	v6502_reset(cpu);
	
	printf("Running...\n");
	run(cpu);

	int commandLen;
	HistEvent ev;
	History *hist = history_init();
	history(hist, &ev, H_SETSIZE, 100);
	
	EditLine *el = el_init(currentFileName, stdin, stdout, stderr);
	el_set(el, EL_PROMPT, &prompt);
	el_set(el, EL_SIGNAL, SIGWINCH);
	el_set(el, EL_EDITOR, "emacs");
	el_set(el, EL_HIST, history, hist);
	
	char *command = NULL;
	while (!feof(stdin)) {
		currentLineNum++;
		
		const char *in = el_gets(el, &commandLen);
		if (!in) {
			break;
		}
		
		history(hist, &ev, H_ENTER, in);
		command = realloc(command, commandLen + 1);
		memcpy(command, in, commandLen);
		
		// Trim newline, always the last char
		command[commandLen - 1] = '\0';
		
		if (command[0] == '\0') {
			continue;
		}
		
		if (v6502_handleDebuggerCommand(cpu, command, commandLen, breakpoint_list, table, run, &verbose)) {
			if (v6502_compareDebuggerCommand(command, commandLen, "help")) {
				printf("woz                 Print relevant woz monitor parameters and registers.\n");
			}
			continue;
		}
		else if (v6502_compareDebuggerCommand(command, commandLen, "woz")) {
			printf("KBD  0x%02x\n"
				   "XAML 0x%02x\n"
				   "XAMH 0x%02x\n"
				   "STL  0x%02x\n"
				   "STH  0x%02x\n"
				   "L    0x%02x\n"
				   "H    0x%02x\n"
				   "YSAV 0x%02x\n"
				   "MODE 0x%02x\n",
				   v6502_read(cpu->memory, A1PIA_KEYBOARD_INPUT_REGISTER, NO),
				   v6502_read(cpu->memory, 0x24, NO), // XAML
				   v6502_read(cpu->memory, 0x25, NO), // XAMH
				   v6502_read(cpu->memory, 0x26, NO), // STL
				   v6502_read(cpu->memory, 0x27, NO), // STH
				   v6502_read(cpu->memory, 0x28, NO), // L
				   v6502_read(cpu->memory, 0x29, NO), // H
				   v6502_read(cpu->memory, 0x2A, NO), // YSAV
				   v6502_read(cpu->memory, 0x2B, NO));// MODE
		}
		else if (command[0] != ';') {
			currentLineText = command;
			as6502_executeAsmLineOnCPU(cpu, command, strlen(command));
		}
	}
	
	as6502_destroySymbolTable(table);
	v6502_destroyBreakpointList(breakpoint_list);
	history_end(hist);
	el_end(el);
	free(command);
	pia_destroy(pia);
	v6502_destroyMemory(cpu->memory);
	v6502_destroyCPU(cpu);
	printf("\n");
	return EXIT_SUCCESS;
}
