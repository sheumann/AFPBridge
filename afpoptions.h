#ifndef AFPOPTIONS_H
#define AFPOPTIONS_H

#define fLargeReads             0x0001
#define fForceAFP22             0x0002
#define fFakeSleep              0x0004
#define fIgnoreFileTypeErrors   0x0008
#define fLargeWrites            0x0010

typedef struct AFPOptions {
    char *optString;
    unsigned int flag;
} AFPOptions;

extern AFPOptions afpOptions[];

#endif
