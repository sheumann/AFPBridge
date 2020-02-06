#pragma rtl

#include <appletalk.h>
#include <locator.h>
#include <misctool.h>
#include <tcpip.h>
#include <desk.h>
#include <orca.h>
#include <gsos.h>
#include <scheduler.h>
#include <string.h>
#include "installcmds.h"
#include "aspinterface.h"
#include "asmglue.h"
#include "cmdproc.h"

const char bootInfoString[] = "AFPBridge             v1.0b1";
LongWord version = 0x01006001;      /* in rVersion format */

const char versionMessageString[] = "\pSTH~AFPBridge~Version~";

const char requestNameString[] = "\pTCP/IP~STH~AFPBridge~";

typedef struct VersionMessageRec {
    Word blockLen;
    char nameString[23];
    LongWord dataBlock;
} VersionMessageRec;

extern Word *unloadFlagPtr;
extern void resetRoutine(void);

void PatchAttentionVector(void);
void pollTask(void);
void notificationProc(void);
static pascal Word requestProc(Word reqCode, Long dataIn, Long dataOut);

static struct RunQRec {
    Long reserved1;
    Word period;
    Word signature;
    Long reserved2;
    Byte jml;
    void (*proc)(void);
} runQRec;

#define NOTIFY_SHUTDOWN    0x20
#define NOTIFY_GSOS_SWITCH 0x04

static struct NotificationProcRec {
    Long reserved1;
    Word reserved2;
    Word Signature;
    Long Event_flags;
    Long Event_code;
    Byte jml;
    void (*proc)(void);
} notificationProcRec;

#define SoftResetPtr ((LongWord *)0xE11010)
extern LongWord oldSoftReset;

#define busyFlagPtr ((Byte*)0xE100FF)

#define JML 0x5C

void setUnloadFlag(void) {
    if (*unloadFlagPtr == 0)
        *unloadFlagPtr = 1;
}

int main(void) {
    unsigned int i;
    FSTInfoRecGS fstInfoRec;
    NotifyProcRecGS addNotifyProcRec;
    VersionMessageRec versionMessageRec;

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
    
    versionMessageRec.blockLen = sizeof(versionMessageRec);
    memcpy(versionMessageRec.nameString, versionMessageString,
            sizeof(versionMessageRec.nameString));
    versionMessageRec.dataBlock = version;
    MessageByName(TRUE, (Pointer)&versionMessageRec);
    if (toolerror())
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
    notificationProcRec.Event_flags = NOTIFY_SHUTDOWN | NOTIFY_GSOS_SWITCH;
    notificationProcRec.jml = JML;
    notificationProcRec.proc = notificationProc;
    addNotifyProcRec.pCount = 1;
    addNotifyProcRec.procPointer = (ProcPtr)&notificationProcRec;
    AddNotifyProcGS(&addNotifyProcRec);
    
    AcceptRequests(requestNameString, userid(), &requestProc);

    oldSoftReset = *SoftResetPtr;
    *SoftResetPtr = ((LongWord)&resetRoutine << 8) | JML;
    
    PatchAttentionVector();
    
    return 0;

error:
    setUnloadFlag();
}

/*
 * Install our own attention vector that bypasses the one in the
 * ATalk driver in problematic cases (for session number > 8).
 */
void PatchAttentionVector(void) {
    PFIHooksRec pfiHooksRec;

    pfiHooksRec.async = 0;
    pfiHooksRec.command = pfiHooksCommand;
    pfiHooksRec.hookFlag = 0;      /* get hooks */
    _CALLAT(&pfiHooksRec);
    if (pfiHooksRec.result == 0) {
        jmlOldAttentionVec = (pfiHooksRec.attentionVector << 8) | JML;
        pfiHooksRec.attentionVector = (LongWord)&attentionVec;
        pfiHooksRec.hookFlag = pfiHooksSetHooks;    /* set hooks, GS/OS mode */
        _CALLAT(&pfiHooksRec);
    }
}


#pragma databank 1
void pollTask(void) {
    Word stateReg;
    
    IncBusyFlag();
    stateReg = ForceRomIn();
    
    if (OS_KIND == KIND_GSOS)
        PollAllSessions();
    runQRec.period = 4*60;
    
    RestoreStateReg(stateReg);
    DecBusyFlag();
}
#pragma databank 0


/*
 * Notification procedure called at shutdown time or when switching to GS/OS.
 *
 * We try to notify the servers that we're closing the connections at shutdown.
 * This only works if Marinetti is still active, i.e. if its own shutdown
 * notification procedure hasn't run yet.
 *
 * When switching back from P8, we reinstall our attention vector patch.
 */
#pragma databank 1
void notificationProc(void) {
    Word stateReg;

    IncBusyFlag();
    stateReg = ForceRomIn();

    if (notificationProcRec.Event_code & NOTIFY_GSOS_SWITCH) {
        PatchAttentionVector();
    }
    if (notificationProcRec.Event_code & NOTIFY_SHUTDOWN) {
        CloseAllSessions(aspAttenClosed, TRUE);
    }

    RestoreStateReg(stateReg);
    DecBusyFlag();
}
#pragma databank 0

#pragma databank 1
void handleDisconnect(void) {
    Word stateReg;

    IncBusyFlag();
    stateReg = ForceRomIn();

    CloseAllSessions(aspAttenClosed, FALSE);

    RestoreStateReg(stateReg);
    DecBusyFlag();
}
#pragma databank 0

/*
 * Request procedure called by Marinetti with its notifications.
 * If the network has gone down, we immediately close all sessions.
 * We check the busy flag to avoid doing this within our code
 * (which runs with the busy flag set), although I don't think that
 * can really happen with current versions of Marinetti.
 */
#pragma databank 1
#pragma toolparms 1
static pascal Word requestProc(Word reqCode, Long dataIn, Long dataOut) {
    if (reqCode == TCPIPSaysNetworkDown) {
        if (*busyFlagPtr) {
            SchAddTask(&handleDisconnect);
        } else {
            handleDisconnect();
        }
    }
    
    return 0;
}
#pragma toolparms 0
#pragma databank 0
