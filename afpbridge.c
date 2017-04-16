#pragma rtl

#include <appletalk.h>
#include <locator.h>
#include <misctool.h>
#include <tcpip.h>
#include <desk.h>
#include <orca.h>
#include <gsos.h>
#include "installcmds.h"
#include "aspinterface.h"
#include "asmglue.h"

const char bootInfoString[] = "AFPBridge             v1.0b1";

extern Word *unloadFlagPtr;
extern void resetRoutine(void);

FSTInfoRecGS fstInfoRec;

void pollTask(void);
void notificationProc(void);

static struct RunQRec {
    Long reserved1;
    Word period;
    Word signature;
    Long reserved2;
    Byte jml;
    void (*proc)(void);
} runQRec;

static struct NotificationProcRec {
    Long reserved1;
    Word reserved2;
    Word Signature;
    Long Event_flags;
    Long Event_code;
    Byte jml;
    void (*proc)(void);
} notificationProcRec;

NotifyProcRecGS addNotifyProcRec;

LongWord *SoftResetPtr = (LongWord *)0xE11010;
extern LongWord oldSoftReset;

#define JML 0x5C

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

    /*
     * Load Marinetti.
     * We may get an error if the TCPIP init isn't loaded yet, but we ignore it.
     * The tool stub is still loaded in that case, which is enough for now.
     */
    LoadOneTool(54, 0x0200);
    if (toolerror() && toolerror() != terrINITNOTFOUND)
        goto error;
    
    ShowBootInfo(bootInfoString, NULL);

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
    runQRec.jml = JML;
    runQRec.proc = pollTask;
    AddToRunQ((Pointer)&runQRec);
    
    notificationProcRec.Signature = 0xA55A;
    notificationProcRec.Event_flags = 0x20; /* shutdown */
    notificationProcRec.jml = JML;
    notificationProcRec.proc = notificationProc;
    addNotifyProcRec.pCount = 1;
    addNotifyProcRec.procPointer = (ProcPtr)&notificationProcRec;
    AddNotifyProcGS(&addNotifyProcRec);
    
    oldSoftReset = *SoftResetPtr;
    *SoftResetPtr = ((LongWord)&resetRoutine << 8) | JML;
    
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


/*
 * Notification procedure called at shutdown time.
 * We try to notify the servers that we're closing the connections.
 * This only works if Marinetti is still active, i.e. if its own
 * shutdown notification procedure hasn't run yet.
 */
#pragma databank 1
void notificationProc(void) {
    Word stateReg;

    IncBusyFlag();
    stateReg = ForceRomIn();

    CloseAllSessions(0);

    RestoreStateReg(stateReg);
    DecBusyFlag();
}
#pragma databank 0
