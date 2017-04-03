#pragma noroot

#include <types.h>
#include <AppleTalk.h>
#include "asmglue.h"
#include "cmdproc.h"

extern LongWord completionRtn;

typedef struct NewCmd {
    Word cmdNum;
    void (*cmdAddr)(void);
} NewCmd;

NewCmd newCmds[] = {
    {aspGetStatusCommand, cmdProc},
    {aspOpenSessionCommand, cmdProc},
    {aspCloseSessionCommand, cmdProc},
    {aspCommandCommand, cmdProc},
    {aspWriteCommand, cmdProc},
    {0, 0}
};

LongWord *cmdTable = (LongWord *)0xE1D600;

#define MAX_CMD rpmFlushSessionCommand

LongWord oldCmds[MAX_CMD + 1];  /* holds old entries for commands we changed */

ATGetInfoRec getInfoRec;

void installCmds(void) {
    Word savedStateReg;
    NewCmd *cmd;
    
    getInfoRec.async = 0;
    getInfoRec.command = atGetInfoCommand;
    _CALLAT(&getInfoRec);
    completionRtn = getInfoRec.completionRtn;

    savedStateReg = ForceLCBank2();
    
    for (cmd = newCmds; cmd->cmdNum != 0; cmd++) {
        oldCmds[cmd->cmdNum] = cmdTable[cmd->cmdNum];
        cmdTable[cmd->cmdNum] = 
            (oldCmds[cmd->cmdNum] & 0xFF000000) | (LongWord)cmd->cmdAddr;
    }
    
    RestoreStateReg(savedStateReg);
}
