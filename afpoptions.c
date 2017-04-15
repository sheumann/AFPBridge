#pragma noroot

#include "afpoptions.h"

AFPOptions afpOptions[] = 
{
    /*12345678901234567890123456789012 <- max length = 32 */
    {"AFP over TCP", 0},
    {"AFP over TCP (LargeReads)", fLargeReads},
    {"AFP over TCP (AFP2.2)", fForceAFP22},
    {"AFP over TCP (LargeReads,AFP2.2)", fLargeReads | fForceAFP22},
    {0, 0}
};
