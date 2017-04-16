#ifndef ASMGLUE_H
#define ASMGLUE_H

#include <types.h>

extern Word ForceLCBank2(void);
extern Word ForceRomIn(void);
extern void RestoreStateReg(Word);

void IncBusyFlag(void) inline(0, 0xE10064);
void DecBusyFlag(void) inline(0, 0xE10068);

#define OS_KIND (*(Byte*)0xE100BC)
#define KIND_P8     0x00
#define KIND_GSOS   0x01

#endif
