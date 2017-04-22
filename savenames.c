#include <string.h>
#include <appletalk.h>
#include "aspinterface.h"
#include "asmglue.h"

/*
 * This code is responsible for saving away the server name and zone at
 * the time of an FILogin(2) call, and then writing them into the result
 * records returned by FIListSessions2.
 *
 * This code is needed because FIListSessions2 has a bug that will cause
 * these fields of its result records to contain garbage for AFP-over-TCP
 * connections and in some cases also wrong values for concurrent 
 * AFP-over-AppleTalk sessions.
 */

#define SERVER_NAME_SIZE 32
#define ZONE_NAME_SIZE   33

typedef struct NameRec {
    unsigned char serverName[SERVER_NAME_SIZE];
    unsigned char zoneName[ZONE_NAME_SIZE];
} NameRec;

#define MAX_ASP_SESSION_NUM 8
#define MAX_PFI_SESSIONS    8

NameRec aspSessionNames[MAX_ASP_SESSION_NUM + 1];
NameRec dsiSessionNames[MAX_SESSIONS];

Byte aspActiveFlags[MAX_ASP_SESSION_NUM + 1] = {0};
Byte dsiActiveFlags[MAX_SESSIONS] = {0};
Byte extraActive = 0;

static unsigned char emptyStr[1] = {0};

typedef struct ListSessions2Buffer {
    Byte refNum;
    Byte slotDrive;
    char volName[28];
    Word volID;
    char serverName[32];
    char zoneName[33];
} ListSessions2Buffer;


/* Count number of active sessions (may overestimate) */
static unsigned int countSess(void) {
    unsigned int activeCount;
    unsigned int i;
    
    activeCount = extraActive;
    for (i = 0; i <= MAX_ASP_SESSION_NUM; i++) {
        activeCount += aspActiveFlags[i];
    }
    for (i = 0; i < MAX_SESSIONS; i++) {
        activeCount += aspActiveFlags[i];
    }
    return activeCount;
}

/*
 * This is called following an FILogin or FILogin2 call, to save the names.
 */
#pragma databank 1
void SaveNames(PFILogin2Rec *commandRec) {
    unsigned char *serverName, *zoneName;
    unsigned int i;
    unsigned int strLen;
    NameRec *nameRec;
    Word stateReg;
    
    stateReg = ForceRomIn();

    /* 
     * Don't save names for failed connections, but if we get a
     * "too many sessions" error when there aren't really too many sessions,
     * then change it to something more appropriate (this can happen when
     * the ASP/DSI layer returns a network error).
     */
    if (commandRec->result != 0 && commandRec->result != pfiLoginContErr) {
        if (commandRec->result == pfiTooManySessErr
            && countSess() < MAX_PFI_SESSIONS)
        {
            commandRec->result = pfiUnableOpenSessErr;
        }
        goto ret;
    }
    
    if (commandRec->result == 0 && commandRec->sessRefID >= SESSION_NUM_START) {
        sessionTbl[commandRec->sessRefID - SESSION_NUM_START].loggedIn = TRUE;
    }
    
    if (commandRec->sessRefID <= MAX_ASP_SESSION_NUM) {
        nameRec = &aspSessionNames[commandRec->sessRefID];
        aspActiveFlags[commandRec->sessRefID] = 1;
    } else if (commandRec->sessRefID >= SESSION_NUM_START) {
        nameRec = &dsiSessionNames[commandRec->sessRefID - SESSION_NUM_START];
        dsiActiveFlags[commandRec->sessRefID - SESSION_NUM_START] = 1;
    } else {
        extraActive = 255;
        goto ret;
    }
    
    /* Get the names (or default to empty strings if not provided) */
    if (commandRec->command == pfiLogin2Command) {
        serverName = commandRec->serverName;
        zoneName = commandRec->zoneName;
    } else {
        serverName = emptyStr;
        zoneName = emptyStr;
    }
    
    memset(nameRec, 0, sizeof(*nameRec));
    strLen = serverName[0];
    if (strLen >= SERVER_NAME_SIZE)
        strLen = SERVER_NAME_SIZE - 1;
    memcpy(&nameRec->serverName, serverName, strLen + 1);
    nameRec->serverName[0] = strLen;
    strLen = zoneName[0];
    if (strLen >= ZONE_NAME_SIZE)
        strLen = ZONE_NAME_SIZE - 1;
    memcpy(&nameRec->zoneName, zoneName, strLen + 1);
    nameRec->zoneName[0] = strLen;

ret:
    RestoreStateReg(stateReg);
}
#pragma databank 0

#pragma databank 1
void InsertCorrectNames(PFIListSessions2Rec *commandRec) {
    unsigned int i;
    ListSessions2Buffer *resultBuf;
    NameRec *nameRec;
    Word stateReg;
    
    stateReg = ForceRomIn();
    
    if (commandRec->result != 0 && commandRec->result != pfiBufferToSmallErr)
        goto ret;
    
    resultBuf = (ListSessions2Buffer *)commandRec->bufferPtr;
    for (i = 0; i < commandRec->entriesRtn; i++) {
        if (resultBuf[i].refNum <= MAX_ASP_SESSION_NUM) {
            nameRec = &aspSessionNames[resultBuf[i].refNum];
        } else if (resultBuf[i].refNum >= SESSION_NUM_START) {
            nameRec = &dsiSessionNames[resultBuf[i].refNum - SESSION_NUM_START];
        } else {
            continue;
        }
        
        memcpy(&resultBuf[i].serverName, nameRec, sizeof(*nameRec));
    }

ret:
    RestoreStateReg(stateReg);
}
#pragma databank 0

#pragma databank 1
void PostLoginCont(PFILoginContRec *commandRec) {
    Word stateReg;
    
    stateReg = ForceRomIn();

    if (commandRec->result == 0 && commandRec->sessRefID >= SESSION_NUM_START) {
        sessionTbl[commandRec->sessRefID - SESSION_NUM_START].loggedIn = TRUE;
    }

    RestoreStateReg(stateReg);
}
#pragma databank 0
