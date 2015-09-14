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

#define ROM_START	0xF000
#define ROM_SIZE	0x00FF
#define MAX_INSTRUCTION_LEN		32

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

static int printSingleInstruction(v6502_cpu *cpu, uint16_t address) {
	char instruction[MAX_INSTRUCTION_LEN];
	int instructionLength;
	dis6502_stringForInstruction(instruction, MAX_INSTRUCTION_LEN, cpu->memory->bytes[address], cpu->memory->bytes[address + 2], cpu->memory->bytes[address + 1]);
	instructionLength = v6502_instructionLengthForOpcode(cpu->memory->bytes[address]);
	
	printf("0x%04x: ", address);
	
	switch (instructionLength) {
		case 1: {
			printf("%02x      ", cpu->memory->bytes[address]);
		} break;
		case 2: {
			printf("%02x %02x   ", cpu->memory->bytes[address], cpu->memory->bytes[address + 1]);
		} break;
		case 3: {
			printf("%02x %02x %02x", cpu->memory->bytes[address], cpu->memory->bytes[address + 1], cpu->memory->bytes[address + 2]);
		} break;
		default: {
			printf("        ");
		} break;
	}
	
	printf(" - %s\n", instruction);
	
	return instructionLength;
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
	
	// Set the reset vector
	v6502_write(cpu->memory, v6502_memoryVectorResetLow, ROM_START & 0xFF);
	v6502_write(cpu->memory, v6502_memoryVectorResetHigh, ROM_START >> 8);
	
	// Attach PIA
	pia_map(cpu->memory);
	
	v6502_reset(cpu);
	
	while (!faulted) {
		v6502_step(cpu);
		//printSingleInstruction(cpu, cpu->pc);
	}
	
	v6502_destroyMemory(cpu->memory);
	v6502_destroyCPU(cpu);
}
