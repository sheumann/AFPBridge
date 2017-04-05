#pragma rtl

#include <appletalk.h>
#include <locator.h>
#include <desk.h>
#include <orca.h>
#include "installcmds.h"
#include "aspinterface.h"
#include "asmglue.h"

void pollTask(void);

static struct RunQRec {
    Long reserved1;
    Word period;
    Word signature;
    Long reserved2;
    Byte jml;
    void (*proc)(void);
} runQRec;


int main(void) {
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

error:
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
