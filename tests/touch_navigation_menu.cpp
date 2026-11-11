// Exercise navigation gestures through the real menu machine, no assets/SDL.
#include "TouchNavigation.h"
#include "Menu.h"
#include "Input.h"
#include <cstdio>
#include <cstdlib>
DWORD CInput::m_menuMoveSound=0, CInput::m_menuBackSound=0, CInput::m_menuErrorSound=0;
DWORD CInput::m_menuSoundVolume=0, CInput::m_menuChangeSound=0, CInput::m_menuSelectSound=0;
int Sound_PlaySampleWithParameters(unsigned short,int,int,int,int,int) { return 0; }
static int cancellations;
extern "C" void Input_CancelTouchNavigation(void) { ++cancellations; }
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
static unsigned Bits(TouchNavigation::Action action) {
    return action == TouchNavigation::None ? 0 : 1u << (action-1);
}
static unsigned Swipe(float x,float y) {
    TouchNavigation t;
    t.Event(0,1,.5f,.5f,0,1200,800);
    return Bits(t.Event(2,1,x,y,100,1200,800));
}
int main() {
    Menu language{}, next{}, parent{};
    Menu_Init(&language,nullptr,1,0,&parent,nullptr,1,0,1);
    for (int i=0;i<3;++i) Menu_AddItemType2(&language,nullptr,(short)i,&next,nullptr,(short)i);
    Menu_ValidateCursor(&language,0);
    Menu_Update(&language,0);
    CHECK(language.cursor==0);
    // Finger up moves the language selection down one. Finger down reverses.
    CHECK(Menu_Update(&language,Swipe(.5f,.3f))==nullptr);
    CHECK(language.cursor==1);
    CHECK(Menu_Update(&language,Swipe(.5f,.7f))==nullptr);
    CHECK(language.cursor==0);
    TouchNavigation tap; tap.Event(0,1,.5f,.5f,0,1200,800);
    CHECK(Menu_Update(&language,Bits(tap.Event(2,1,.5f,.5f,100,1200,800)))==&next);
    CHECK(cancellations>0);
    Menu_Update(&language,0);
    TouchNavigation back; back.Event(0,1,.4f,.5f,0,1200,800); back.Event(0,2,.6f,.5f,10,1200,800);
    CHECK(Menu_Update(&language,Bits(back.Event(2,1,.4f,.5f,50,1200,800)))==nullptr);
    CHECK(Menu_Update(&language,Bits(back.Event(2,2,.6f,.5f,60,1200,800)))==&parent);
    // In a vertical menu, horizontal swipes change an adjustable option.
    Menu slider{};Menu_Init(&slider,nullptr,1,0,nullptr,nullptr,1,0,1);
    Menu_AddItemType3(&slider,nullptr,1,11,5,0,0,nullptr,0);
    Menu_ValidateCursor(&slider,0);Menu_Update(&slider,0);
    Menu_Update(&slider,Swipe(.3f,.5f));CHECK(slider.items[0].max==6);
    Menu_Update(&slider,Swipe(.7f,.5f));CHECK(slider.items[0].max==5);
    std::puts("touch_navigation_menu: all checks passed");
}
