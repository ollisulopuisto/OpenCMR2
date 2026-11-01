#include "DriverNamePatch.h"
#include "AIHelper.h"
#include "GameInfo.h"
#include <cstdio>
#include <cstring>
#include <string>
extern char g_aiNames[15][20];
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CHECK(!strcmp(g_aiNames[0],"mcrae")&&!strcmp(g_aiNames[14],"utzman"));
 CGameInfo::m_gameInfo.field_0x14=0;
 CHECK(!strcmp(CAIHelper::GetNameForID(15),"mcrae"));
 char names[15][20]={};std::string input="OPENCMR2-DRIVER-NAMES-1\n";
 for(int i=0;i<15;i++)input+=std::to_string(i)+"\tdriver "+std::to_string(i)+"\n";
 CHECK(DriverNamePatch::Parse(input,names));CHECK(!strcmp(names[14],"driver 14"));
 char saved[15][20];memcpy(saved,names,sizeof names);
 for(auto bad:{input+"extra\n",input.substr(0,input.size()-12),std::string("wrong\n"),input+std::string(1,'\0')}){
  CHECK(!DriverNamePatch::Parse(bad,names));CHECK(!memcmp(saved,names,sizeof names));
 }
 std::string longName=input;longName.replace(longName.find("driver 0"),8,std::string(20,'x'));
 CHECK(!DriverNamePatch::Parse(longName,names));CHECK(!memcmp(saved,names,sizeof names));
 std::string duplicate=input;duplicate.replace(duplicate.find("1\t"),2,"0\t");
 CHECK(!DriverNamePatch::Parse(duplicate,names));CHECK(!memcmp(saved,names,sizeof names));
 puts("original fallback, bounded IDs and atomic ordered name parser: pass");
}
