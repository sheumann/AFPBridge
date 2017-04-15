#ifndef AFPOPTIONS_H
#define AFPOPTIONS_H

#define fLargeReads     0x0001
#define fForceAFP22     0x0002

typedef struct AFPOptions {
    char *zoneString;
    unsigned int flags;
} AFPOptions;

extern AFPOptions afpOptions[];

#endif
