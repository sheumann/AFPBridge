#pragma noroot

#include "afpoptions.h"

AFPOptions afpOptions[] = 
{
    /*12345678901234567890123456789012 <- max length = 32 */
    {"AFP over TCP", 0},
    {"AFP over TCP (LargeReads)", fLargeReads},
    {"AFP over TCP (AFP2.2)", fForceAFP22},
    {"AFP over TCP (LargeReads,AFP2.2)", fLargeReads | fForceAFP22},
    {"AFP over TCP (AFP2.2,FakeSleep)", fForceAFP22 | fFakeSleep},
    {"AFP over TCP (LR,AFP2.2,FS)", fLargeReads | fForceAFP22 | fFakeSleep},
    {0, 0}
};
