#include <cstring>
#include <cstdio>
extern char g_aiNames[15][20];
int main(){const char *names[]={"t. makinen","c. mcrae","r. burns","d. auriol","c. sainz","j. kankkunen","m. gronholm","f. loix","f. delecour","h. rovanpera","t. radstrom","t. gardemeister","p. liatti","p. solberg","b. thiry"};
for(int i=0;i<15;i++)if(strcmp(g_aiNames[i],names[i])||strlen(g_aiNames[i])>=20)return 1;puts("grounded15slotname-onlytable: pass");}
