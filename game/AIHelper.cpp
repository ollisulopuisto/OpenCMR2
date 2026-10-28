#include "AIHelper.h"
#include "GameInfo.h"
#include "RallyData.h"

// GLOBAL: CMR2 0x005166b4
// Names only, transcribed from the supplied real-driver patch static table.
// Its executable was not run; performance and car data are unchanged.
char g_aiNames[15][20] = {
    "t. makinen",
    "c. mcrae",
    "r. burns",
    "d. auriol",
    "c. sainz",
    "j. kankkunen",
    "m. gronholm",
    "f. loix",
    "f. delecour",
    "h. rovanpera",
    "t. radstrom",
    "t. gardemeister",
    "p. liatti",
    "p. solberg",
    "b. thiry"};

// FUNCTION: CMR2 0x00407f80
int CAIHelper::ResolveNameIndex(int id)
{
    unsigned char uVar1;

    if ((id < 0) || (5 < id))
        id = 0;

    uVar1 = CGameInfo::GetConfiguredDifficulty();
    if (uVar1 == 2)
        return id;

    uVar1 = CGameInfo::GetConfiguredDifficulty();
    if (uVar1 == 1)
        return id + 5;

    return id + 10;
}

// FUNCTION: CMR2 0x00407ec0
char *CAIHelper::GetNameForID(int id)
{
    if (CGameInfo::GetConfiguredGameMode() != 5)
    {
        if (CGameInfo::GetConfiguredGameMode() != 6)
        {
            if (CGameInfo::GetConfiguredGameMode() != 7)
            {
                if (id < 0 || id > 15)
                    id = 0;

                return g_aiNames[id];
            }
        }
    }

    // The subtraction is kept as its own local: MSVC then holds the state byte
    // in cl and the difference in eax, matching the original.
    int offset = id - RallyDataState();

    return g_aiNames[CAIHelper::ResolveNameIndex(offset)];
}
