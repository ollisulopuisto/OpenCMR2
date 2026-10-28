#ifndef OPENCMR2_CAR_DISPLAY_NAMES_H
#define OPENCMR2_CAR_DISPLAY_NAMES_H
#include <array>
#include <string>
#include <sstream>
namespace CarDisplayNames {
// Archive order from CFrontend::GetArchiveDirectoryEntry, not car-family IDs.
inline const char *Slot(int i) {
 static const char *slots[]={"foc","f99","fa1","fa2","mit","ma1","ma2","ma3","cor","sub","206","sea","int","ia1","ia2","sie","min","6r4","str","205","pum","esc"};
 return i>=0&&i<22?slots[i]:nullptr;
}
using Names=std::array<std::array<std::string,2>,22>;
inline bool Parse(const std::string &text,Names &out) {
 Names staged{};std::istringstream input(text);std::string line;
 if(!std::getline(input,line)||line!="OPENCMR2-CAR-NAMES-1")return false;
 int count=0;
 while(std::getline(input,line)){
  auto a=line.find('\t'),b=a==line.npos?a:line.find('\t',a+1);
  if(a==line.npos||b==line.npos||line.find('\t',b+1)!=line.npos)return false;
  int id=-1;for(int i=0;i<22;i++)if(line.substr(0,a)==Slot(i))id=i;
  if(id<0||!staged[id][0].empty())return false;
  auto full=line.substr(a+1,b-a-1),brief=line.substr(b+1);
  if(full.empty()||full.size()>39||brief.empty()||brief.size()>23)return false;
  for(unsigned char c:full+brief)if(c<32||c>126||c=='%')return false;
  staged[id]={full,brief};++count;
 }
 if(!count)return false;out=staged;return true;
}
// Only the known car-label ranges in the known localized text tables.
// Other text tables reuse the same IDs (including a printf format at 0x98).
inline int TextSlot(int index,int count,int mode,bool &brief) {
 int base=mode==0&&count==0x215?0x98:mode==3&&count==0x104?0xc2:-1;
 if(base<0||index<base||index>=base+44)return -1;
 brief=index>=base+22;return (index-base)%22;
}
}
#endif
