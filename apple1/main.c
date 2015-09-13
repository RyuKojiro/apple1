//
//  main.c
//  apple1
//
//  Created by Daniel Loffgren on 9/12/15.
//  Copyright (c) 2015 Daniel Loffgren. All rights reserved.
//

#include <v6502/cpu.h>
#include <v6502/mem.h>

void fault(void *ctx, const char *e) {
	(*(int *)ctx)++;
}

int main(int argc, const char * argv[])
{
	int faulted = 0;
	
	v6502_cpu *cpu = v6502_createCPU();
	cpu->memory = v6502_createMemory(0xFFFF);
	cpu->fault_callback = fault;
	cpu->fault_context = &faulted;
	
	v6502_reset(cpu);
	
	while (!faulted) {
		v6502_step(cpu);
	}
	
	
	v6502_destroyMemory(cpu->memory);
	v6502_destroyCPU(cpu);
}
