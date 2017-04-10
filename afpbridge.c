#pragma rtl

#include <appletalk.h>
#include <locator.h>
#include <desk.h>
#include <orca.h>
#include <gsos.h>
#include "installcmds.h"
#include "aspinterface.h"
#include "asmglue.h"

extern Word *unloadFlagPtr;

FSTInfoRecGS fstInfoRec;

void pollTask(void);

static struct RunQRec {
    Long reserved1;
    Word period;
    Word signature;
    Long reserved2;
    Byte jml;
    void (*proc)(void);
} runQRec;


void setUnloadFlag(void) {
    if (*unloadFlagPtr == 0)
        *unloadFlagPtr = 1;
}

int main(void) {
    unsigned int i;

    /*
     * Check for presence of AppleShare FST.  We error out and unload
     * if it's not present.  Our code doesn't directly depend on the
     * AppleShare FST, but in practice it's not useful without it.
     * This also ensures lower-level AppleTalk stuff is present.
     */
    fstInfoRec.pCount = 2;
    fstInfoRec.fileSysID = 0;
    for (i = 1; fstInfoRec.fileSysID != appleShareFSID; i++) {
        fstInfoRec.fstNum = i;
        GetFSTInfoGS(&fstInfoRec);
        if (toolerror() == paramRangeErr)
            goto error;
    }

    LoadOneTool(54, 0x0300);    /* load Marinetti 3.0+ */
    if (toolerror())
        goto error;

    /*
     * Put Marinetti in the default TPT so its tool stub won't be unloaded,
     * even if UnloadOneTool is called on it.  Programs may still call
     * TCPIPStartUp and TCPIPShutDown, but those don't actually do
     * anything, so the practical effect is that Marinetti will always
     * be available once its init has loaded (which may not have happened
     * yet when this init loads).
     */
    SetDefaultTPT();

    RamForbid();
    installCmds();
    RamPermit();

    runQRec.period = 1;
    runQRec.signature = 0xA55A;
    runQRec.jml = 0x5C;
    runQRec.proc = pollTask;
    AddToRunQ((Pointer)&runQRec);
    
    return;

error:
    setUnloadFlag();
    return;
}


#pragma databank 1
void pollTask(void) {
    Word stateReg;
    
    IncBusyFlag();
    stateReg = ForceRomIn();
    
    PollAllSessions();
    runQRec.period = 4*60;
    
    RestoreStateReg(stateReg);
    DecBusyFlag();
}
#pragma databank 0
