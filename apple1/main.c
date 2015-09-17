//
//  main.c
//  apple1
//
//  Created by Daniel Loffgren on 9/12/15.
//  Copyright (c) 2015 Daniel Loffgren. All rights reserved.
//

#include <v6502/cpu.h>
#include <v6502/mem.h>
#include <stdio.h>
#include "pia.h"
#include <unistd.h>
#include <dis6502/reverse.h>
#include <v6502/log.h>

#define ROM_START		0xF000
#define ROM_SIZE		0x00FF
#define RESET_VECTOR	0xFF00

void fault(void *ctx, const char *e) {
	(*(int *)ctx)++;
}

static void loadRomFile(v6502_memory *mem, const char *fname, uint16_t address) {
	FILE *f = fopen(fname, "r");
	
	if (!f) {
		fprintf(stderr, "Could not read from \"%s\"!\n", fname);
		return;
	}
	
	uint8_t byte;
	uint16_t offset = 0;
	
	while (fread(&byte, 1, 1, f)) {
		mem->bytes[address + (offset++)] = byte;
	}
	
	fprintf(stderr, "loaded \"%s\" at 0x%04x\n", fname, address);
	
	fclose(f);
}

uint8_t romMirrorCallback(struct _v6502_memory *memory, uint16_t offset, int trap, void *context) {
	return memory->bytes[offset % ROM_SIZE];
}

int main(int argc, const char * argv[])
{
	int faulted = 0;
	
	v6502_cpu *cpu = v6502_createCPU();
	cpu->memory = v6502_createMemory(v6502_memoryStartCeiling + 1);
	cpu->fault_callback = fault;
	cpu->fault_context = &faulted;
	
	// Load Woz Monitor
	for (uint16_t start = ROM_START;
		 start < v6502_memoryStartCeiling && start >= ROM_START;
		 start += ROM_SIZE + 1) {
		loadRomFile(cpu->memory, "apple1.rom", start);
		//v6502_map(cpu->memory, start, ROM_SIZE, romMirrorCallback, NULL, NULL);
	}
	
	// Attach PIA
	a1pia *pia = pia_create(cpu->memory);
	
	v6502_reset(cpu);
	
	FILE *asmfile = fopen("runtime.s", "w");
	while (!faulted) {
		v6502_step(cpu);
		dis6502_printAnnotatedInstruction(asmfile, cpu, cpu->pc);
		v6502_printCpuState(asmfile, cpu);
	}
	fclose(asmfile);
	
	pia_destroy(pia);
	v6502_destroyMemory(cpu->memory);
	v6502_destroyCPU(cpu);
}
