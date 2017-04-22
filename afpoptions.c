#pragma noroot

#include "afpoptions.h"

/*
 * These are option codes that can be used in the zone string, e.g.
 * "AFP over TCP (LR,22)".  Note that the zone string is limited to 32
 * characters.  With 2-character options, we can use up to six at once.
 */
AFPOptions afpOptions[] = 
{
    {"LR", fLargeReads},
    {"LW", fLargeWrites},
    {"22", fForceAFP22},
    {"FS", fFakeSleep},
    {"IE", fIgnoreFileTypeErrors},
    {0, 0}
};
