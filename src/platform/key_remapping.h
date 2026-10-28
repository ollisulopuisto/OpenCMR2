// Per-launch gameplay bindings. Saved Controller.rcf and text/menu keys stay intact.
#ifndef OPENCMR2_KEY_REMAPPING_H
#define OPENCMR2_KEY_REMAPPING_H
#include "platform/platform.h"
#include <string>
#include <cstring>
#include <cctype>
struct KeyRemappingOptions {
    int keys[2][9];
    KeyRemappingOptions(){for(auto &row:keys)for(int &k:row)k=-1;}
    static int Action(const std::string &s){
        static const char *names[]={"left","right","accelerate","brake","handbrake","gear-up","gear-down","camera","look-back"};
        for(int i=0;i<9;i++)if(s==names[i])return i;return -1;
    }
    bool Consume(int &i,int argc,char **argv,std::string &error){
        if(std::strcmp(argv[i],"--bind-key"))return false;
        if(++i>=argc){error="--bind-key requires [1:|2:]ACTION=KEY; see --help-controls";return true;}
        std::string value=argv[i];int keyboard=0;
        if(value.size()>1&&value[1]==':'){if(value[0]!='1'&&value[0]!='2'){error="Keyboard must be 1 or 2";return true;}keyboard=value[0]-'1';value.erase(0,2);}
        auto eq=value.find('=');int action=Action(value.substr(0,eq));
        if(eq==value.npos||action<0){error="Unknown binding action; see --help-controls";return true;}
        std::string name=value.substr(eq+1);int key=0;
        if(name!="none"){
            // SDL scancode names are physical key positions, not text translation.
            SDL_Scancode scan=SDL_GetScancodeFromName(name.c_str());key=Keys_FromScancode(scan);
            if(!key||key==1){error="Unknown/unsupported or reserved Escape key; see --help-controls";return true;}
        }
        keys[keyboard][action]=key;return true;
    }
    bool Validate(std::string &error)const{
        for(auto &row:keys)for(int i=0;i<9;i++)for(int j=i+1;j<9;j++)if(row[i]>0&&row[i]==row[j]){error="Two explicit actions use the same key on one keyboard";return false;}return true;
    }
    void Apply(int keyboard,unsigned char *out)const{
        if(keyboard<0||keyboard>1)return;
        const int *row=keys[keyboard];
        // Apply simultaneously so swaps do not lose either action.
        for(int i=0;i<9;i++)if(row[i]>=0)out[i]=(unsigned char)row[i];
        for(int i=0;i<9;i++)if(row[i]<0)for(int j=0;j<9;j++)if(row[j]>0&&out[i]==row[j])out[i]=0;
    }
};
inline const char *KeyRemappingHelp(){return
"Keyboard remapping:\n"
"  In-game: Options > Controls, choose a keyboard, select an action and\n"
"  release the confirm key before pressing the new key. Escape cancels\n"
"  capture. Existing Controls save flow persists in-game edits.\n"
"  --bind-key [1:|2:]ACTION=KEY  Repeat for per-launch keyboard overrides\n"
"  --help-controls             Show controls help without starting game\n"
"  Actions: left,right,accelerate,brake,handbrake,gear-up,gear-down,\n"
"           camera,look-back\n"
"  Keys: SDL physical scancode names, e.g. A, W, Space, Left Shift,\n"
"        Up, Left, Keypad 8; quote names with spaces. none unbinds.\n"
"  Keyboard 1 is default. Keyboard 2 is the second local keyboard layout.\n"
"  No preset is applied. Unspecified actions keep saved keys except a key\n"
"  reassigned to another action is cleared there. Explicit duplicates fail;\n"
"  swaps work. Escape is reserved. Menu arrows/Enter/Escape, text entry and\n"
"  Alt+Enter stay unchanged. CLI bindings are not written to Controller.rcf\n"
"  and take priority over in-game edits for this launch only. Omit to undo.\n"
"  Example: --bind-key left=A --bind-key right=D --bind-key accelerate=W\n"
"           --bind-key brake=S\n";}
#endif
