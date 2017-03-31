#ifndef ASMGLUE_H
#define ASMGLUE_H

#include <types.h>

extern Word ForceLCBank2(void);
extern Word ForceRomIn(void);
extern void RestoreStateReg(Word);

void IncBusyFlag(void) inline(0, 0xE10064);
void DecBusyFlag(void) inline(0, 0xE10068);

#endif
