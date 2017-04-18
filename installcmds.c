#pragma noroot

#include <types.h>
#include <AppleTalk.h>
#include "asmglue.h"
#include "cmdproc.h"
#include "installcmds.h"

typedef struct NewCmd {
    Word cmdNum;
    void (*cmdAddr)(void);
    LongWord *jslOldCmdLocation;
} NewCmd;

NewCmd newCmds[] = {
    {aspGetStatusCommand, cmdProc, NULL},
    {aspOpenSessionCommand, cmdProc, NULL},
    {aspCloseSessionCommand, cmdProc, NULL},
    {aspCommandCommand, cmdProc, NULL},
    {aspWriteCommand, cmdProc, NULL},
    {nbpLookupNameCommand, nbpCmdProc, NULL},
    {pfiLoginCommand, pfiLoginCmdProc, &jslOldPFILogin},
    {pfiLogin2Command, pfiLogin2CmdProc, &jslOldPFILogin2},
    {pfiListSessions2Command, pfiListSessions2CmdProc, &jslOldPFIListSessions2},
    {0, 0, 0}
};

LongWord *cmdTable = (LongWord *)0xE1D600;

LongWord oldCmds[MAX_CMD + 1];  /* holds old entries for commands we changed */

#define JSL 0x22

void installCmds(void) {
    Word savedStateReg;
    NewCmd *cmd;

    savedStateReg = ForceLCBank2();
    
    for (cmd = newCmds; cmd->cmdNum != 0; cmd++) {
        oldCmds[cmd->cmdNum] = cmdTable[cmd->cmdNum];
        cmdTable[cmd->cmdNum] = 
            (oldCmds[cmd->cmdNum] & 0xFF000000) | (LongWord)cmd->cmdAddr;
        if (cmd->jslOldCmdLocation != NULL)
            *cmd->jslOldCmdLocation = JSL | (oldCmds[cmd->cmdNum] << 8);
    }
    
    RestoreStateReg(savedStateReg);
}
