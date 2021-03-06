/**

	File:		dbgaux.c

	Project:	DCPU-16 Toolchain
	Component:	LibDCPU-vm-dbg

	Authors:	James Rhodes
			José Manuel Díez

	Description:	Defines auxilary functions for the debugger.

**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bstrlib.h>
#include <dcpu.h>
#include <dcpuhook.h>
#include <hwio.h>
#include <hwlem1802.h>
#include <hwtimer.h>
#include <imap.h>

#define MAX_BREAKPOINTS 100

uint16_t flash_size = 0;
uint16_t flash[0x10000];
uint16_t breakpoints[MAX_BREAKPOINTS];
uint16_t breakpoints_num;
extern vm_t* vm;

void ddbg_help(bstring section)
{
	if (biseq(section, bfromcstr("general")))
	{
		printf("Available commands:\n");
		printf("- break\n");
		printf("- load\n");
		printf("- run\n");
		printf("- continue\n");
		printf("- inspect\n");
		printf("- quit\n");
		printf("Type `help <command>' to get help about a particular command.\n");
	}
	else if (biseq(section, bfromcstr("break")))
	{
		printf("Manipulates breakpoints.\n");
		printf("Available commands: add, delete, list\n");
		printf("Syntax: `break <command> <path>:<address>'\n");
		printf("Alternate syntax: `break <command> <address>'\n");
		printf("Example: break add memory:0xbeef\n");
	}
	else if (biseq(section, bfromcstr("load")))
	{
		printf("Loads words from a file to the VM.\n");
	}
	else if (biseq(section, bfromcstr("run")) || biseq(section, bfromcstr("continue")))
	{
		printf("Sets vm->halted to false, runs the VM.\n");
	}
	else if (biseq(section, bfromcstr("inspect")))
	{
		printf("Returns information about devices.\n");
		printf("Available commands: cpu, memory.\n");
		printf("Syntax: `inspect <command> [<arg1>] [<arg2>]'\n");
		printf("Note: `inspect memory' takes one or two arguments; the first argument is the start address and the second argument is the number of words to be dumped.\n");
	}
	else if (biseq(section, bfromcstr("quit")))
	{
		printf("Gracefully quits the debugger.\n");
	}
}

void ddbg_cycle_hook(vm_t* vm, uint16_t pos)
{
	int i = 0;

	for (i = 0; i < breakpoints_num; i++)
	{
		if (vm->pc == breakpoints[i] && vm->pc != 0xFFFF)
		{
			vm->halted = true;
			printf("Breakpoint hit at 0x%04X.\n", breakpoints[i]);
		}
	}
}

void ddbg_load(bstring path)
{
	FILE* load;
	unsigned int a = 0, i = 0;
	int cread;
	bool uread = true;

	load = fopen(path->data, "rb");

	if (load == NULL)
	{
		printf("Unable to load %s from disk.\n", path->data);
		exit(1);
	}

	for (i = 0; i < 0x20000; i++)
	{
		cread = fgetc(load);

		if (cread == -1) break;

		if (uread)
			cread <<= 8;

		flash[a] += ((cread << 8) | (cread >> 8));

		if (!uread)
			a += 1;

		uread = !uread;
	}

	fclose(load);

	printf("Loaded 0x%04X words from %s.\n", a, path->data);
	flash_size = a;
}

void ddbg_create_vm()
{
	vm = vm_create();
	vm_hook_register(vm, &ddbg_cycle_hook, HOOK_ON_CYCLE);
	printf("Created VM.\n");
}

void ddbg_flash_vm()
{
	vm_flash(vm, flash);
	printf("Flashed memory.\n");
}

void ddbg_run_vm()
{
	vm->halted = false;
	vm_execute(vm);
	printf("\n");
}

void ddbg_continue_vm()
{
	vm->halted = false;
	vm_execute(vm);
	printf("\n");
}

void ddbg_attach(bstring hw)
{
	if (biseq(hw, bfromcstr("lem1802")))
		vm_hw_lem1802_init(vm);
	else if (biseq(hw, bfromcstr("keyboard")))
		vm_hw_io_init(vm);
	else if (biseq(hw, bfromcstr("clock")))
		vm_hw_timer_init(vm);
	else
		printf("Unrecognized hardware.\n");
}

void ddbg_add_breakpoint(bstring file, int index)
{
	if (!biseq(file, bfromcstr("memory")))
	{
		printf("Only memory breakpoints can be set at this time (use memory:address).\n");
		return;
	}

	if (index < 0)
	{
		printf("Index must be greater than 0.\n");
		return;
	}

	breakpoints[breakpoints_num++] = index;
	printf("Breakpoint added at 0x%04X.\n", index);
}

void ddbg_delete_breakpoint(bstring file, int index)
{
	int i = 0;
	bool found = false;

	if (!biseq(file, bfromcstr("memory")))
	{
		printf("Only memory breakpoints can be set at this time (use memory:address).\n");
		return;
	}

	if (index < 0)
	{
		printf("Index must be greater than 0.\n");
		return;
	}

	for (i = 0; i < breakpoints_num; i++)
	{
		if (breakpoints[i] == index)
		{
			breakpoints[i] = 0xFFFF;
			found = true;
		}
	}

	if (found == true)
	{
		printf("Breakpoint at %s:%d removed.\n", bstr2cstr(file, 0), index);
	}
	else
	{
		printf("There was no breakpoint at %s:%d.\n", bstr2cstr(file, 0), index);
	}
}

void ddbg_breakpoints_list()
{
	int i = 0;

	printf("%d breakpoints loaded.\n", breakpoints_num);

	for (i = 0; i < breakpoints_num; i++) printf("- at 0x%04X\n", breakpoints[i]);
}

void ddbg_dump_state()
{
	fprintf(stderr, "A:   0x%04X	 [A]:	0x%04X\n", vm->registers[REG_A], vm->ram[vm->registers[REG_A]]);
	fprintf(stderr, "B:   0x%04X	 [B]:	0x%04X\n", vm->registers[REG_B], vm->ram[vm->registers[REG_B]]);
	fprintf(stderr, "C:   0x%04X	 [C]:	0x%04X\n", vm->registers[REG_C], vm->ram[vm->registers[REG_C]]);
	fprintf(stderr, "X:   0x%04X	 [X]:	0x%04X\n", vm->registers[REG_X], vm->ram[vm->registers[REG_X]]);
	fprintf(stderr, "Y:   0x%04X	 [Y]:	0x%04X\n", vm->registers[REG_Y], vm->ram[vm->registers[REG_Y]]);
	fprintf(stderr, "Z:   0x%04X	 [Z]:	0x%04X\n", vm->registers[REG_Z], vm->ram[vm->registers[REG_Z]]);
	fprintf(stderr, "I:   0x%04X	 [I]:	0x%04X\n", vm->registers[REG_I], vm->ram[vm->registers[REG_I]]);
	fprintf(stderr, "J:   0x%04X	 [J]:	0x%04X\n", vm->registers[REG_J], vm->ram[vm->registers[REG_J]]);
	fprintf(stderr, "PC:  0x%04X	 SP:	0x%04X\n", vm->pc, vm->sp);
	fprintf(stderr, "EX:  0x%04X	 IA:	0x%04X\n", vm->ex, vm->ia);
}

void ddbg_dump_ram(int start, int difference)
{
	int i = 0;

	if (start < 0 || difference < 0)
	{
		printf("Invalid parameters provided to 'inspect memory'.");
		return;
	}

	if (difference == 0)
		difference = 32;

	for (i = 0; i < difference; i++)
	{
		if (i % 8 == 0)
			printf("%04X: ", start + i);

		printf("%04X ", vm->ram[start + i]);

		if ((i + 1) % 8 == 0)
			printf("\n");
	}

	printf("\n");
}

void ddbg_disassemble(int start, int difference)
{
	int i = 0;
	uint16_t inst, op_a, op_b, val;
	struct instruction_mapping* map_inst;
	struct register_mapping* map_op_a;
	struct register_mapping* map_op_b;

	if (start < 0 || difference < 0)
	{
		printf("Invalid parameters provided to 'disassemble'.");
		return;
	}

	if (difference == 0)
		difference = (flash_size - start);

	for (i = 0; i < difference; i++)
	{
		val = vm->ram[start + i];
		inst = INSTRUCTION_GET_OP(vm->ram[start + i]);
		op_a = INSTRUCTION_GET_A(vm->ram[start + i]);
		op_b = INSTRUCTION_GET_B(vm->ram[start + i]);

		map_inst = get_instruction_by_value(inst, op_b);
		map_op_a = get_register_by_value(op_a);
		map_op_b = get_register_by_value(op_b);

		printf("0x%04X (0x%04X): ", start + i, vm->ram[start + i]);
		if (map_inst != NULL)
		{
			printf("%s ", map_inst->name);
			if (op_b == NXT)
				printf("[0x%04X] ", vm->ram[start + (++i)]);
			else if (op_b == NXT_LIT)
				printf("0x%04X ", vm->ram[start + (++i)]);
			else if (map_op_b != NULL)
				printf("%s ", map_op_b->name);
			else
				printf("0x%04X", op_b);
			if (op_a == NXT)
				printf("[0x%04X] ", vm->ram[start + (++i)]);
			else if (op_a == NXT_LIT)
				printf("0x%04X", vm->ram[start + (++i)]);
			else if (map_op_a != NULL)
				printf("%s ", map_op_a->name);
			else
				printf("0x%04X", op_a);
		}
		else if (val == 0x0)
			printf("<null>"); // No data here.
		else
			printf("DAT");
		printf("\n");
	}

	printf("\n");
}
