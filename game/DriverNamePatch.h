#ifndef OPENCMR2_DRIVER_NAME_PATCH_H
#define OPENCMR2_DRIVER_NAME_PATCH_H
#include <string>
#include <sstream>
#include <cstring>
namespace DriverNamePatch {
// Complete, ordered replacement only. A malformed file cannot partially apply.
inline bool Parse(const std::string &text,char (&out)[15][20]) {
 std::istringstream input(text);std::string line;char staged[15][20]={};
 if(!std::getline(input,line))return false;
 if(!line.empty()&&line.back()=='\r')line.pop_back();
 if(line!="OPENCMR2-DRIVER-NAMES-1")return false;
 for(int i=0;i<15;i++){
  if(!std::getline(input,line))return false;
  if(!line.empty()&&line.back()=='\r')line.pop_back();
  std::string prefix=std::to_string(i)+"\t";
  if(line.compare(0,prefix.size(),prefix)!=0)return false;
  std::string name=line.substr(prefix.size());
  if(name.empty()||name.size()>=20)return false;
  for(unsigned char c:name)if(c<32||c>126)return false;
  std::memcpy(staged[i],name.data(),name.size());
 }
 if(std::getline(input,line))return false;
 std::memcpy(out,staged,sizeof staged);return true;
}
}
#endif
