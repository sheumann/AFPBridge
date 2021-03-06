#pragma cdev cdevMain

#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <locator.h>
#include <misctool.h>
#include <gsos.h>
#include <orca.h>
#include <AppleTalk.h>
#include <quickdraw.h>
#include <window.h>
#include <control.h>
#include <resources.h>
#include <stdfile.h>
#include <lineedit.h>
#include <memory.h>
#include <desk.h>
#include <menu.h>
#include <finder.h>
#include <tcpip.h>
#include "afpoptions.h"
#include "afpurlparser.h"
#include "strncasecmp.h"
#include "cdevutil.h"

#define MachineCDEV     1
#define BootCDEV        2
#define InitCDEV        4
#define CloseCDEV       5
#define EventsCDEV      6
#define CreateCDEV      7
#define AboutCDEV       8
#define RectCDEV        9
#define HitCDEV         10
#define RunCDEV         11
#define EditCDEV        12

#define serverAddressTxt    2
#define urlLine             3
#define saveAliasBtn        4
#define connectBtn          1
#define optionsPopUp        6
#define trianglePic         7

#define saveFilePrompt      100

#define optionsMenu                         300
#define afpOverTCPOptionsItem               301
#define useLargeReadsItem                   302
#define forceAFP22Item                      303
#define fakeSleepItem                       304
#define useLargeWritesItem                  305
#define ignoreErrorsSettingFileTypesItem    306

#define fstMissingError     3000
#define noEasyMountError    3001
#define tempFileError       3002
#define aliasFileError      3003
#define tempFileNameError   3004
#define saveAliasError      3005
#define noAFPBridgeError    3006
#define noAFPBridgeWarning  3007

#define EM_filetype     0xE2
#define EM_auxtype      0xFFFF

FSTInfoRecGS fstInfoRec;

char urlBuf[257];

WindowPtr wPtr = NULL;

/* System 6.0-style EasyMount rec, as documented in its FTN. */
typedef struct EasyMountRec {
    char entity[97];
    char volume[28];
    char user[32];
    char password[8];
    char volpass[8];
} EasyMountRec;

EasyMountRec easyMountRec;

typedef struct GSString1024 {
    Word length;
    char text[1024];
} GSString1024;

typedef struct ResultBuf1024 {
    Word  bufSize;
    GSString1024 bufString;
} ResultBuf1024;

CreateRecGS createRec;
OpenRecGS openRec;
IORecGS writeRec;
RefNumRecGS closeRec;
NameRecGS destroyRec;
RefInfoRecGS getRefInfoRec;

ResultBuf1024 filename = { sizeof(ResultBuf1024) };

char tempFileName[] = "AFPMounter.Temp";

finderSaysBeforeOpenIn fsboRec;

GSString32 origNameString;
SFReplyRec2 sfReplyRec;

Word modifiers = 0;

char zoneBuf[ZONE_MAX + 1];
char AFPOverTCPZone[] = "AFP over TCP";

/* Default flags for AFP over TCP connections */
unsigned int flags = fLargeReads;

void fillEasyMountRec(char *server, char *zone, char *volume, char *user,
                      char *password, char *volpass)
{
    unsigned int i;
    char *next;

    memset(&easyMountRec, 0, sizeof(easyMountRec));

    i = 0;
    next = server;
    easyMountRec.entity[i++] = strlen(next);
    while (*next != 0 && i < sizeof(easyMountRec.entity) - 2) {
        easyMountRec.entity[i++] = *next++;
    }
    next = "AFPServer";
    easyMountRec.entity[i++] = strlen(next);
    while (*next != 0 && i < sizeof(easyMountRec.entity) - 1) {
        easyMountRec.entity[i++] = *next++;
    }
    next = zone;
    easyMountRec.entity[i++] = strlen(next);
    while (*next != 0 && i < sizeof(easyMountRec.entity)) {
        easyMountRec.entity[i++] = *next++;
    }

    easyMountRec.volume[0] = strlen(volume);
    strncpy(&easyMountRec.volume[1], volume, sizeof(easyMountRec.volume) - 1);
    
    easyMountRec.user[0] = strlen(user);
    strncpy(&easyMountRec.user[1], user, sizeof(easyMountRec.user) - 1);
    
    strncpy(&easyMountRec.password[0], password, sizeof(easyMountRec.password));
    
    strncpy(&easyMountRec.volpass[0], volpass, sizeof(easyMountRec.volpass));
}

int deleteAlias(GSString255Ptr file, Boolean overwrite)
{
    if (overwrite) {
        /*
         * Zero out the data before deleting, so password isn't left
         * in the unallocated disk block.
         */
        memset(&easyMountRec, 0, sizeof(easyMountRec));

        openRec.pCount = 3;
        openRec.pathname = file;
        openRec.requestAccess = writeEnable;
        OpenGS(&openRec);
        if (toolerror())
            goto destroy;
    
        writeRec.pCount = 5;
        writeRec.refNum = openRec.refNum;
        writeRec.dataBuffer = (Pointer)&easyMountRec;
        writeRec.requestCount = sizeof(EasyMountRec);
        writeRec.cachePriority = cacheOff;
        WriteGS(&writeRec);
        if (toolerror())
            goto destroy;
    
        closeRec.pCount = 1;
        closeRec.refNum = openRec.refNum;
        CloseGS(&closeRec);
        if (toolerror())
            goto destroy;
    }

destroy:
    destroyRec.pCount = 1;
    destroyRec.pathname = file;
    DestroyGS(&destroyRec);
    return toolerror();
}

int writeAlias(GSString255Ptr file)
{
    int err;

    createRec.pCount = 6;
    createRec.pathname = file;
    createRec.access = readEnable|writeEnable|renameEnable|destroyEnable;
    createRec.fileType = EM_filetype;
    createRec.auxType = EM_auxtype;
    createRec.storageType = standardFile;
    createRec.eof = sizeof(EasyMountRec);
    CreateGS(&createRec);
    if ((err = toolerror()))
        return err;
    
    openRec.pCount = 3;
    openRec.pathname = file;
    openRec.requestAccess = writeEnable;
    OpenGS(&openRec);
    if ((err = toolerror())) {
        deleteAlias(file, FALSE);
        return err;
    }
    
    writeRec.pCount = 5;
    writeRec.refNum = openRec.refNum;
    writeRec.dataBuffer = (Pointer)&easyMountRec;
    writeRec.requestCount = sizeof(EasyMountRec);
    writeRec.cachePriority = cacheOn;
    WriteGS(&writeRec);
    if ((err = toolerror())) {
        deleteAlias(file, TRUE);
        return err;
    }
    
    closeRec.pCount = 1;
    closeRec.refNum = openRec.refNum;
    CloseGS(&closeRec);
    if ((err = toolerror())) {
        deleteAlias(file, TRUE);
        return err;
    }
    
    return 0;
}

void finalizeURLParts(AFPURLParts *urlParts)
{
    unsigned int i;

    if (urlParts->server == NULL)
        urlParts->server = "";
    if (urlParts->zone == NULL)
        urlParts->zone = "*";
    if (urlParts->username == NULL)
        urlParts->username = "";
    if (urlParts->password == NULL)
        urlParts->password = "";
    if (urlParts->auth == NULL)
        urlParts->auth = "";
    if (urlParts->volpass == NULL)
        urlParts->volpass = "";
    if (urlParts->volume == NULL)
        urlParts->volume = "";

    /*
     * Guess the protocol if it's not explicitly specified: 
     * If server name contains a dot it's probably an IP address
     * or domain name, so use TCP.  Otherwise use AppleTalk.
     */
    if (urlParts->protocol == proto_unknown) {
        if (strchr(urlParts->server, '.') != NULL) {
            urlParts->protocol = proto_TCP;
        } else {
            urlParts->protocol = proto_AT;
        }
    }

    /*
     * Assemble zone name for TCP, including any options.
     * Note that the generated zone name can't be over 32 characters.
     */
    if (urlParts->protocol == proto_TCP) {
        strcpy(zoneBuf, AFPOverTCPZone);
        if (flags != 0) {
            strcat(zoneBuf, " (");
        
            for (i = 0; afpOptions[i].optString != NULL; i++) {
                if (afpOptions[i].flag & flags) {
                    strcat(zoneBuf, afpOptions[i].optString);
                    strcat(zoneBuf, ",");
                }
            }
            zoneBuf[strlen(zoneBuf) - 1] = ')';
        }
        urlParts->zone = zoneBuf;
    }
}

AFPURLParts prepareURL(char *url) {
    int result;
    AFPURLParts urlParts;

    urlParts = parseAFPURL(url);
    result = validateAFPURL(&urlParts);
    if (result != 0) {
        AlertWindow(awResource+awButtonLayout, NULL, result);
        urlParts.protocol = proto_invalid;
        return urlParts;
    }
    finalizeURLParts(&urlParts);
    return urlParts;
}

void ConnectOrSave(AFPURLParts* urlParts, GSString255Ptr file, Boolean connect)
{
    Word recvCount;

    fillEasyMountRec(urlParts->server, urlParts->zone, urlParts->volume,
                     urlParts->username, urlParts->password, urlParts->volpass);
    
    if (writeAlias(file) != 0) {
        AlertWindow(awResource+awButtonLayout, NULL, 
                    connect ? tempFileError : aliasFileError);
        return;
    }
    
    if (connect) {
        recvCount = 0;
        fsboRec.pCount = 7;
        fsboRec.pathname = (pointer)file;
        fsboRec.zoomRect = 0;
        fsboRec.filetype = EM_filetype;
        fsboRec.auxtype = EM_auxtype;
        fsboRec.modifiers = modifiers;
        fsboRec.theIconObj = 0;
        fsboRec.printFlag = 0;
        SendRequest(finderSaysBeforeOpen, sendToAll+stopAfterOne, 0,
                    (Long)&fsboRec, (Ptr)&recvCount);

        deleteAlias(file, TRUE);

        if (recvCount == 0) {
            AlertWindow(awResource+awButtonLayout, NULL, noEasyMountError);
            return;
        }
    }
}

Boolean checkVersions(void)
{
    static struct {
        Word blockLen;
        char nameString[23];
    } messageRec = { sizeof(messageRec), "\pSTH~AFPBridge~Version~" };
    
    /* Check if AFPBridge is active (any version is OK) */
    MessageByName(FALSE, (Pointer)&messageRec);
    if (toolerror())
        return FALSE;
    
    /* Check for Marinetti (AFPBridge will keep it active if installed) */
    if (!TCPIPStatus() || toolerror())
        return FALSE;
    
    return TRUE;
}

void DoConnect(void)
{
    Word i;
    char *lastColon;
    AFPURLParts urlParts;
    Boolean completedOK = FALSE;
    CtlRecHndl ctl;

    GetLETextByID(wPtr, urlLine, (StringPtr)&urlBuf);
    urlParts = prepareURL(urlBuf+1);
    if (urlParts.protocol == proto_invalid)
        goto fixcaret;
    
    if (urlParts.protocol == proto_TCP && !checkVersions()) {
        AlertWindow(awResource+awButtonLayout, NULL, noAFPBridgeError);
        goto fixcaret;
    }

    /* Generate the path name for the temp file in same dir as the CDev */
    getRefInfoRec.pCount = 3;
    getRefInfoRec.refNum = GetOpenFileRefNum(0); /* current resource file */
    getRefInfoRec.pathname = (ResultBuf255Ptr)&filename;
    GetRefInfoGS(&getRefInfoRec);
    if (toolerror())
        goto err;
    if (filename.bufString.length > filename.bufSize - 5 - strlen(tempFileName))
        goto err;
    filename.bufString.text[filename.bufString.length] = 0;
    lastColon = strrchr(filename.bufString.text, ':');
    if (lastColon == NULL)
        goto err;
    strcpy(lastColon + 1, tempFileName);
    filename.bufString.length = strlen(filename.bufString.text);
    
    ConnectOrSave(&urlParts, (GSString255Ptr)&filename.bufString, TRUE);
    completedOK = TRUE;

err:
    /* Most error cases here should be impossible or very unlikely. */
    if (!completedOK)
        AlertWindow(awResource+awButtonLayout, NULL, tempFileNameError);

fixcaret:
    /* Work around issue where parts of the LE caret may flash out of sync */
    ctl = GetCtlHandleFromID(wPtr, urlLine);
    LEDeactivate((LERecHndl) GetCtlTitle(ctl));
    if (FindTargetCtl() == ctl) {
        LEActivate((LERecHndl) GetCtlTitle(ctl));
    }
}

void DoSave(void)
{
    Boolean loadedSF = FALSE, startedSF = FALSE, completedOK = FALSE;
    Handle dpSpace;
    AFPURLParts urlParts;
    CtlRecHndl ctl;

    GetLETextByID(wPtr, urlLine, (StringPtr)&urlBuf);
    urlParts = prepareURL(urlBuf+1);
    if (urlParts.protocol == proto_invalid)
        return;
    
    if (urlParts.protocol == proto_TCP && !checkVersions()) {
        if (AlertWindow(awResource+awButtonLayout, NULL, noAFPBridgeWarning)==0)
            return;
    }

    /* Load Standard File toolset if necessary */
    if (!SFStatus() || toolerror()) {
        if (toolerror())
            loadedSF = TRUE;
        LoadOneTool(0x17, 0x0303);  /* Standard File */
        if (toolerror())
            goto err;
        dpSpace = NewHandle(0x0100, GetCurResourceApp(),
                attrLocked|attrFixed|attrNoCross|attrBank|attrPage, 0x000000);
        if (toolerror())
            goto err;
        SFStartUp(GetCurResourceApp(), (Word) *dpSpace);
        startedSF = TRUE;
    }
    
    /* Initially proposed file name = volume name */
    origNameString.length = strlen(urlParts.volume);
    strcpy(origNameString.text, urlParts.volume); /* OK since VOLUME_MAX < 32 */
    
    /* Get the file name */
    memset(&sfReplyRec, 0, sizeof(sfReplyRec));
    sfReplyRec.nameRefDesc = refIsNewHandle;
    sfReplyRec.pathRefDesc = refIsPointer;
    sfReplyRec.pathRef = (Ref) &filename;
    SFPutFile2(GetMasterSCB() & scbColorMode ? 160 : 25, 40, 
               refIsResource, saveFilePrompt,
               refIsPointer, (Ref)&origNameString, &sfReplyRec);
    if (toolerror()) {
        if (sfReplyRec.good)
            DisposeHandle((Handle)sfReplyRec.nameRef);
        goto err;
    }
    
    /* Save the file, unless user canceled */
    if (sfReplyRec.good) {
        DisposeHandle((Handle)sfReplyRec.nameRef);
        deleteAlias((GSString255Ptr)&filename.bufString, FALSE);
        ConnectOrSave(&urlParts, (GSString255Ptr)&filename.bufString, FALSE);
    }
    completedOK = TRUE;

err:
    if (!completedOK)
        AlertWindow(awResource+awButtonLayout, NULL, saveAliasError);
    if (startedSF) {
        SFShutDown();
        DisposeHandle(dpSpace);
    }
    if (loadedSF)
        UnloadOneTool(0x17);

    /* Work around issue where parts of the LE caret may flash out of sync */
    ctl = GetCtlHandleFromID(wPtr, urlLine);
    LEDeactivate((LERecHndl) GetCtlTitle(ctl));
    if (FindTargetCtl() == ctl) {
        LEActivate((LERecHndl) GetCtlTitle(ctl));
    }
}

void DoHit(long ctlID, CtlRecHndl ctlHandle)
{
    CtlRecHndl oldMenuBar;
    Word menuItem;

    if (!wPtr)  /* shouldn't happen */
        return;

    if (ctlID == connectBtn) {
        DoConnect();
    } else if (ctlID == saveAliasBtn) {
        DoSave();
    } else if (ctlID == optionsPopUp) {
        oldMenuBar = GetMenuBar();
        SetMenuBar(ctlHandle);
        menuItem = GetCtlValue(ctlHandle);
        
        if (menuItem == useLargeReadsItem) {
            flags ^= fLargeReads;
            CheckMItem((flags & fLargeReads) ? TRUE : FALSE, useLargeReadsItem);
        } else if (menuItem == forceAFP22Item) {
            flags ^= fForceAFP22;
            CheckMItem((flags & fForceAFP22) ? TRUE : FALSE, forceAFP22Item);
        } else if (menuItem == fakeSleepItem) {
            flags ^= fFakeSleep;
            CheckMItem((flags & fFakeSleep) ? TRUE : FALSE, fakeSleepItem);
        } else if (menuItem == useLargeWritesItem) {
            flags ^= fLargeWrites;
            CheckMItem((flags & fLargeWrites) ? TRUE : FALSE,
                       useLargeWritesItem);
        } else if (menuItem == ignoreErrorsSettingFileTypesItem) {
            flags ^= fIgnoreFileTypeErrors;
            CheckMItem((flags & fIgnoreFileTypeErrors) ? TRUE : FALSE,
                       ignoreErrorsSettingFileTypesItem);
        }
        
        SetCtlValue(afpOverTCPOptionsItem, ctlHandle);
        SetMenuBar(oldMenuBar);
    }
    
    return;
}

long DoMachine(void)
{
    unsigned int i;

    /* Check for presence of AppleShare FST. */
    fstInfoRec.pCount = 2;
    fstInfoRec.fileSysID = 0;
    for (i = 1; fstInfoRec.fileSysID != appleShareFSID; i++) {
        fstInfoRec.fstNum = i;
        GetFSTInfoGS(&fstInfoRec);
        if (toolerror() == paramRangeErr) {
            InitCursor();
            AlertWindow(awResource+awButtonLayout, NULL, fstMissingError);
            return 0;
        }
    }
    
    return 1;
}

void DoEdit(Word op)
{
    CtlRecHndl ctl;
    GrafPortPtr port;
    
    if (!wPtr)
        return;
    port = GetPort();
    SetPort(wPtr);
    
    ctl = FindTargetCtl();
    if (toolerror() || GetCtlID(ctl) != urlLine)
        goto ret;

    switch (op) {
    case cutAction:     LECut((LERecHndl) GetCtlTitle(ctl));
                        if (LEGetScrapLen() > 0)
                            LEToScrap();
                        break;
    case copyAction:    LECopy((LERecHndl) GetCtlTitle(ctl));
                        if (LEGetScrapLen() > 0)
                            LEToScrap();
                        break;
    case pasteAction:   LEFromScrap();
                        LEPaste((LERecHndl) GetCtlTitle(ctl));
                        break;
    case clearAction:   LEDelete((LERecHndl) GetCtlTitle(ctl));
                        break;
    }

ret:
    SetPort(port);
}

void DoCreate(WindowPtr windPtr)
{
    int mode;
    
    wPtr = windPtr;
    mode = (GetMasterSCB() & scbColorMode) ? 640 : 320;
    NewControl2(wPtr, resourceToResource, mode);
}

LongWord cdevMain (LongWord data2, LongWord data1, Word message)
{
    long result = 0;

    switch(message) {
    case MachineCDEV:   result = DoMachine();               break;
    case HitCDEV:       DoHit(data2, (CtlRecHndl)data1);    break;
    case EditCDEV:      DoEdit(data1 & 0xFFFF);             break;
    case CreateCDEV:    DoCreate((WindowPtr)data1);         break;
    case CloseCDEV:     wPtr = NULL;                        break;
    case EventsCDEV:    /* Now done in assembly for speed.  Equivalent to: */
                        /* modifiers = ((EventRecordPtr)data1)->modifiers; */
                        break;
    }

ret:
    FreeAllCDevMem();
    return result;
}
