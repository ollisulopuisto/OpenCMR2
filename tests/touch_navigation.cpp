#include "TouchNavigation.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
using T = TouchNavigation;
static T::Action swipe(float x, float y) {
    T t; t.Event(0,1,.5f,.5f,0,1200,800);
    CHECK(t.Event(1,1,x,y,50,1200,800)==T::None);
    return t.Event(2,1,x,y,100,1200,800);
}
int main() {
    TouchPicture picture{800,600,false};
    CHECK(picture.Outside(.01f,.5f,1200,600));
    CHECK(!picture.Outside(.5f,.5f,1200,600));
    picture = TouchPicture{1200,600,false}; // later menu must not inherit old bounds
    CHECK(!picture.Outside(.01f,.5f,1200,600));
    picture = TouchPicture{800,600,true}; // stretched movie fills its window
    CHECK(!picture.Outside(.01f,.5f,1200,600));
    picture = TouchPicture{1200,600,false};
    CHECK(picture.Outside(.5f,.01f,800,800));
    TouchPictureRect movie{420,0,1080,1080};
    TouchPicture movieFrame{1920,1080,false};
    CHECK(movie.Outside(movieFrame,.1f,.5f,1200,800));
    CHECK(!movie.Outside(movieFrame,.5f,.5f,1200,800));
    CHECK(movie.Outside(movieFrame,.5f,.01f,1200,800));
    CHECK(TouchPictureRect{}.Outside(movieFrame,.5f,.5f,1200,800));
    CHECK(swipe(.3f,.5f)==T::Right);
    CHECK(swipe(.7f,.5f)==T::Left);
    CHECK(swipe(.5f,.3f)==T::Down);
    CHECK(swipe(.5f,.7f)==T::Up);
    T t;
    CHECK(t.Event(0,1,.5f,.5f,0,1200,800)==T::None);
    CHECK(t.Event(2,1,.5f,.5f,100,1200,800)==T::Confirm);
    CHECK(t.Event(2,1,.5f,.5f,110,1200,800)==T::None);
    t.Event(0,1,.5f,.5f,200,1200,800);
    t.Event(0,2,.6f,.5f,220,1200,800);
    CHECK(t.Event(2,1,.5f,.5f,250,1200,800)==T::None);
    CHECK(t.Event(2,2,.6f,.5f,270,1200,800)==T::Back);
    // Either finger moving disqualifies a two-finger tap.
    t.Event(0,1,.5f,.5f,300,1200,800);t.Event(0,2,.6f,.5f,310,1200,800);
    t.Event(1,2,.8f,.5f,320,1200,800);
    CHECK(t.Event(2,2,.8f,.5f,330,1200,800)==T::None);
    CHECK(t.Event(2,1,.5f,.5f,340,1200,800)==T::None);
    // A long hold and a small ambiguous drag never confirm.
    t.Event(0,1,.5f,.5f,400,1200,800);
    CHECK(t.Event(2,1,.5f,.5f,1200,1200,800)==T::None);
    CHECK(swipe(.53f,.5f)==T::None);
    // A swipe returning to its anchor stays a swipe and locks its first axis.
    t.Event(0,1,.5f,.5f,1300,1200,800);t.Event(1,1,.3f,.5f,1350,1200,800);
    CHECK(t.Event(2,1,.5f,.8f,1400,1200,800)==T::Right);
    // Transition with another finger still down cancels the whole gesture.
    t.Event(0,1,.5f,.5f,1500,1200,800);t.Cancel();
    t.Event(0,2,.6f,.5f,1510,1200,800);
    CHECK(t.Event(2,1,.5f,.5f,1520,1200,800)==T::None);
    CHECK(t.Event(2,2,.6f,.5f,1530,1200,800)==T::None);
    t.Event(0,1,.5f,.5f,1600,1200,800);
    CHECK(t.Event(2,1,.5f,.5f,1650,1200,800)==T::Confirm);
    // Three-finger sequences are ignored, and full 64-bit IDs stay distinct.
    t.Event(0,1,.5f,.5f,1700,1200,800);t.Event(0,0x100000001LL,.5f,.5f,1710,1200,800);
    t.Event(0,3,.5f,.5f,1720,1200,800);
    CHECK(t.Event(2,1,.5f,.5f,1730,1200,800)==T::None);
    CHECK(t.Event(2,0x100000001LL,.5f,.5f,1740,1200,800)==T::None);
    CHECK(t.Event(2,3,.5f,.5f,1750,1200,800)==T::None);
    t.Event(0,9,.5f,.5f,1800,1200,800);t.Reset();
    CHECK(t.Event(2,9,.5f,.5f,1810,1200,800)==T::None);
    t.Event(0,10,.5f,.5f,1820,1200,800);
    CHECK(t.Event(2,10,.5f,.5f,1830,1200,800)==T::Confirm);
    t.Event(0,11,.01f,.5f,1900,1200,800,true);
    CHECK(t.Event(2,11,.01f,.5f,1950,1200,800,true)==T::None);
    t.Event(0,12,.5f,.5f,2000,1200,800);
    CHECK(t.Event(2,12,.01f,.5f,2050,1200,800,true)==T::None);
    std::puts("touch_navigation: all checks passed");
}
