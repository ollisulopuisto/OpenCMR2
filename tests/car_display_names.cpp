#include "port/types.h"
#include "CarDisplayNames.h"
#include "Frontend.h"
#include "Game.h"
#include <cstring>
#include <cstdio>
static bool enabled=true;
extern "C" const char *__wrap_Sys_GetCarDisplayName(int slot,BOOL brief){return enabled&&slot==2?(brief?"Fabia WRC Red Bull":"Skoda Fabia WRC Red Bull"):nullptr;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 CarDisplayNames::Names n{};std::string valid="OPENCMR2-CAR-NAMES-1\nfa1\tSkoda Fabia WRC Red Bull\tFabia WRC Red Bull\n";
 CHECK(CarDisplayNames::Parse(valid,n));CHECK(n[2][0]=="Skoda Fabia WRC Red Bull"&&n[0][0].empty());auto saved=n;
 for(auto bad:{valid+valid.substr(valid.find("fa1")),valid+"bad\tx\ty\n",std::string("wrong\n"),valid+"mit\t%p\ty\n",valid+"sub\t"+std::string(40,'x')+"\ty\n"}){CHECK(!CarDisplayNames::Parse(bad,n));CHECK(n==saved);}
 char original[]="original";char *table[0x42a];for(auto &p:table)p=original;
 CFrontend::m_textStrings=table;CFrontend::m_textFirstId=0;CFrontend::m_textCount=0x215;CGame::SetFrontendResourceMode(0);
 CHECK(!strcmp(CFrontend::GetTextString(0x9a),"Skoda Fabia WRC Red Bull"));CHECK(!strcmp(CFrontend::GetModeSpecificCountryText(2),"Skoda Fabia WRC Red Bull"));CHECK(!strcmp(CFrontend::GetModeSpecificStageText(2),"Fabia WRC Red Bull"));
 CHECK(CFrontend::GetTextString(0x98)==original&&CFrontend::GetTextString(0x99)==original); // unchanged Focus and F99 variants
 CFrontend::m_textFirstId=0x215;CHECK(!strcmp(CFrontend::GetTextString(0x9a),"Skoda Fabia WRC Red Bull")); // language independent
 CFrontend::m_textCount=0x104;CFrontend::m_textFirstId=0;CGame::SetFrontendResourceMode(3);
 CHECK(!strcmp(CFrontend::GetModeSpecificCountryText(2),"Skoda Fabia WRC Red Bull"));CHECK(!strcmp(CFrontend::GetModeSpecificStageText(2),"Fabia WRC Red Bull"));CHECK(CFrontend::GetTextString(0x98)==original);
 enabled=false;CHECK(CFrontend::GetTextString(0xc4)==original);enabled=true;
 CFrontend::m_textCount=0x146;CHECK(CFrontend::GetTextString(0xc4)==original);
 CGame::SetFrontendResourceMode(2);CFrontend::m_textCount=0x215;CHECK(CFrontend::GetTextString(0x9a)==original);
 CHECK(CFrontend::GetTextString(-1)==CFrontend::m_strInvalidTextString&&CFrontend::GetTextString(0x215)==CFrontend::m_strInvalidTextString);
 CHECK(!strcmp((const char*)CFrontend::GetArchiveDirectoryEntry(2),"FA1"));
}
