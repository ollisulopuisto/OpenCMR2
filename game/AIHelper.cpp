#include "AIHelper.h"
#include "GameInfo.h"
#include "RallyData.h"
#include "DriverNamePatch.h"
#include "port/sys.h"

// GLOBAL: CMR2 0x005166b4
char g_aiNames[15][20] = {
    "mcrae",
    "erikson",
    "dugas",
    "baca",
    "tizot",
    "aikita",
    "huber",
    "ingauld",
    "jonsson",
    "truman",
    "jones",
    "kastis",
    "candido",
    "cooper",
    "utzman"};

namespace {
char customNames[15][20];
bool namesChecked=false, namesAvailable=false;
const char *nameStatus="original (not checked)";
char (*NameTable())[20] {
    if (!namesChecked) {
        namesChecked=true;
        nameStatus="original (disabled)";
        if (Sys_GetOption("video.driver_name_overrides",1)!=0) {
            nameStatus="original (sidecar missing)";
            SysFile *file=Sys_OpenFile("driver-names.tsv",SYS_FILE_READ);
            if (file) {
                nameStatus="original (invalid sidecar)";
                DWORD size=Sys_GetFileSize(file);
                std::string content;
                if(size<=1024) {
                    content.resize(size);
                    if(Sys_ReadFile(file,content.data(),size)==size)
                        namesAvailable=DriverNamePatch::Parse(content,customNames);
                }
                Sys_CloseFile(file);
                if(namesAvailable) nameStatus="custom (15 validated names loaded)";
            }
        }
    }
    return namesAvailable ? customNames : g_aiNames;
}
}

void CAIHelper::LogDriverNameStatus() {
    NameTable();
    Sys_Log("options: driver names=%s",nameStatus);
}

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
                if (id < 0 || id >= 15)
                    id = 0;

                return NameTable()[id];
            }
        }
    }

    // The subtraction is kept as its own local: MSVC then holds the state byte
    // in cl and the difference in eax, matching the original.
    int offset = id - RallyDataState();

    int index=CAIHelper::ResolveNameIndex(offset);
    if(index<0 || index>=15) index=0;
    return NameTable()[index];
}
