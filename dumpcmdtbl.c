#include <stdio.h>
#include <types.h>
#include "asmglue.h"

LongWord *cmdTable = (LongWord *)0xE1D600;

LongWord cmdTableCopy[256];

int main(void) {
	int i;
	Word savedStateReg;

	IncBusyFlag();
	savedStateReg = ForceLCBank2();
	for (i = 0; i <= 0xFF; i++) {
		cmdTableCopy[i] = cmdTable[i];
	}
	RestoreStateReg(savedStateReg);
	DecBusyFlag();

	for (i = 0; i <= 0xFF; i++) {
		printf("%02x: %08lx\n", i, cmdTableCopy[i]);
	}
}
