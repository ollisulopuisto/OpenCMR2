#include "WhatsNew.h"
#include <cstdio>
#include <cstring>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main()
{
    int n;
    const WhatsNewLine *lines = WhatsNew_Lines(n);
    CHECK(kWhatsNewRevision >= 1);
    // the draw adapts the step (24 down to a floor of 18 480p units, font 1's
    // line height) so the show-all screen stays on-frame; 18 is the floor cap
    CHECK(n > 0 && n <= 18);
    for (int i = 0; i < n; i++) {
        CHECK(lines[i].revision >= 1 && lines[i].revision <= kWhatsNewRevision);
        CHECK(lines[i].text != NULL);
        CHECK(std::strlen(lines[i].text) > 0 && std::strlen(lines[i].text) <= 72);   // fits 640px at the menu font
    }
    CHECK(WhatsNew_UnseenCount(0) == n);                    // a first run shows everything once
    CHECK(WhatsNew_UnseenCount(kWhatsNewRevision) == 0);    // current revision: nothing new, no auto-show
    CHECK(WhatsNew_UnseenCount(kWhatsNewRevision - 1) >= 1);   // every revision adds lines, so an auto-show always has content
    if (!fails) std::puts("whats_new ok");
    return fails;
}
