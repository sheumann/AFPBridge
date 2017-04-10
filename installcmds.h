#ifndef INSTALLCMDS_H
#define INSTALLCMDS_H

#include <AppleTalk.h>

#define MAX_CMD rpmFlushSessionCommand

extern LongWord oldCmds[MAX_CMD + 1];

void installCmds(void);

#endif
