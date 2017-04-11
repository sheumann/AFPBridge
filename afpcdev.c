#pragma cdev cdevMain

#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <locator.h>
#include <gsos.h>
#include <orca.h>
#include <AppleTalk.h>
#include <quickdraw.h>
#include <window.h>
#include <control.h>
#include "afpurlparser.h"
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

#define fstMissingError     3000
#define noEasyMountError    3001

FSTInfoRecGS fstInfoRec;

char urlBuf[257];

WindowPtr wPtr = NULL;

typedef struct EasyMountRec {
    Word size;
    char entity[97];
    char volume[28];
    char user[32];
    char password[8];
    char volpass[8];
    /* Below fields are new in System 6.0.1 version */
    Word version;  /* 1 = file alias, 2 = 6.0.1-style server alias */
    char volume2[29];
} EasyMountRec;

EasyMountRec easyMountRec;

struct ResultRec {
    Word recvCount;
    Word result;
} resultRec;

char afpOverTCPZone[] = "AFP over TCP";

void fillEasyMountRec(char *server, char *zone, char *volume, char *user,
                      char *password, char *volpass)
{
    unsigned int i;
    char *next;

    memset(&easyMountRec, 0, sizeof(easyMountRec));
    easyMountRec.size = offsetof(EasyMountRec, volume2) + 2 + strlen(volume);
    easyMountRec.version = 2;

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

    easyMountRec.volume2[0] = strlen(volume) + 1;
    easyMountRec.volume2[1] = ':';
    strncpy(&easyMountRec.volume2[2], volume, sizeof(easyMountRec.volume2) - 2);
}

Word tryConnect(enum protocol protocol, AFPURLParts *urlParts)
{
    resultRec.recvCount = 0;
    resultRec.result = 0;

    if (protocol == proto_AT) {
        fillEasyMountRec(urlParts->server, urlParts->zone,
                         urlParts->volume, urlParts->username,
                         urlParts->password, urlParts->volpass);
    } else if (protocol == proto_TCP) {
        fillEasyMountRec(urlParts->server, afpOverTCPZone,
                         urlParts->volume, urlParts->username,
                         urlParts->password, urlParts->volpass);
    } else {
        return atInvalidCmdErr;
    }
    
    SendRequest(0x8000, sendToName+stopAfterOne, (Long)"\pApple~EasyMount~",
                (Long)&easyMountRec, (Ptr)&resultRec);
    
    if (resultRec.recvCount == 0) {
        return atInvalidCmdErr;
    } else {
        return resultRec.result;
    }
}

void connect(AFPURLParts *urlParts)
{
    Word result;

    if (urlParts->protocol == proto_AT || urlParts->protocol == proto_TCP) {
        result = tryConnect(urlParts->protocol, urlParts);
    } else if (urlParts->protocol == proto_unknown) {
        /*
         * If server name contains a dot it's probably an IP address or
         * domain name, so try TCP first.  Otherwise try AppleTalk first.
         * In either case, we proceed to try the other if we get an NBP error.
         */
        if (strchr(urlParts->server, '.') != NULL) {
            if (((result = tryConnect(proto_TCP, urlParts)) & 0xFF00) == 0x0400)
                result = tryConnect(proto_AT, urlParts);
        } else {
            if (((result = tryConnect(proto_AT, urlParts)) & 0xFF00) == 0x0400)
                result = tryConnect(proto_TCP, urlParts);
        }
    } else {
        AlertWindow(awResource, NULL, protoInvalidError);
        return;
    }
    
    if (resultRec.recvCount == 0) {
        AlertWindow(awResource, NULL, noEasyMountError);
        return;
    }
}

void finalizeURLParts(AFPURLParts *urlParts)
{
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
}

void DoConnect(char *url)
{
    AFPURLParts urlParts;
    int validationError;
    
    urlParts = parseAFPURL(url);

    if (validationError = validateAFPURL(&urlParts)) {
        AlertWindow(awResource, NULL, validationError);
        return;
    }
    
    finalizeURLParts(&urlParts);
    connect(&urlParts);
}

void DoHit(long ctlID)
{
    if (ctlID == connectBtn) {
        GetLETextByID(wPtr, urlLine, (StringPtr)&urlBuf);
        DoConnect(urlBuf+1);
    } else if (ctlID == saveAliasBtn) {
        
    }
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
            AlertWindow(awResource, NULL, fstMissingError);
            return 0;
        }
    }
    
    return 1;
}

LongWord cdevMain (LongWord data2, LongWord data1, Word message)
{
    long result = 0;

    switch(message) {
    case MachineCDEV:   result = DoMachine();       break;
    case HitCDEV:       DoHit(data2);               break;
    case InitCDEV:      wPtr = (WindowPtr)data1;    break;
    case CloseCDEV:     wPtr = NULL;                break;
    }

ret:
    FreeAllCDevMem();
    return result;
}
