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

static v6502_cpu *cpu;
static a1pia *pia;

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
	while (!faulted && !pia->signalled) {
		dis6502_printAnnotatedInstruction(asmfile, cpu, cpu->pc);
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
	cpu->memory = v6502_createMemory(v6502_memoryStartCeiling + 1);
	
	v6502_breakpoint_list *breakpoint_list = v6502_createBreakpointList();

	// Load Woz Monitor
	for (uint16_t start = ROM_START;
		start < v6502_memoryStartCeiling && start >= ROM_START;
		start += ROM_SIZE + 1) {
		v6502_loadFileAtAddress(cpu->memory, "apple1.rom", start);
		//v6502_map(cpu->memory, start, ROM_SIZE, romMirrorCallback, NULL, NULL);
	}
	
	// Attach PIA
	pia = pia_create(cpu->memory);
	
	v6502_reset(cpu);
	
	int verbose = 0;
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
		
		if (v6502_handleDebuggerCommand(cpu, command, commandLen, breakpoint_list, run, &verbose)) {
			continue;
		}
		else if (command[0] != ';') {
			as6502_executeAsmLineOnCPU(cpu, command, strlen(command));
		}
	}
	
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
