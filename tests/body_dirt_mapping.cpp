// Pure checks of CarBodyDirt paint-atlas mapping (game/CarBodyDirt.h):
// original stems, path/case handling, the video.body_dirt_extra opt-in list,
// and the deliberate miss for unknown/mod names without it.
#include "CarBodyDirt.h"
#include <cstdio>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main()
{
    using namespace CarBodyDirt;
    // Original atlas stems bind, with paths, extensions and mixed case.
    CHECK(IsPaintAtlas("asidbodf.bmp", ""));
    CHECK(IsPaintAtlas("textures/affdbodf.ppm", ""));
    CHECK(IsPaintAtlas("C:\\CMR2\\cars\\ASIDBODF.BMP", ""));
    CHECK(IsPaintAtlas("amldbodf", ""));
    // Unknown and custom/mod names stay clean without the opt-in.
    CHECK(!IsPaintAtlas("subaru_22b.png", ""));
    CHECK(!IsPaintAtlas(NULL, ""));
    CHECK(!IsPaintAtlas("asid.png", ""));                 // no prefix matching
    CHECK(!IsPaintAtlas("asidbodf2.bmp", ""));            // no suffix matching
    // The opt-in list binds named mod stems (comma-separated, spaces ok, 4 max).
    CHECK(IsPaintAtlas("subaru_22b.png", "subaru_22b"));
    CHECK(IsPaintAtlas("Subaru_22B.PNG", " subaru_22b , other"));
    CHECK(!IsPaintAtlas("other.png", "subaru_22b"));
    CHECK(IsPaintAtlas("s4.png", "s1,s2,s3,s4"));
    CHECK(!IsPaintAtlas("s5.png", "s1,s2,s3,s4"));        // 5th entry ignored
    CHECK(!IsPaintAtlas("s1.png", ""));                   // empty list binds nothing
    // StemOf behaviour.
    char stem[32];
    StemOf("a/b/c/Tex.Name.BMP", stem); CHECK(std::strcmp(stem, "tex") == 0);
    StemOf(NULL, stem); CHECK(stem[0] == 0);
    StemOf("noext", stem); CHECK(std::strcmp(stem, "noext") == 0);
    if (!fails) std::puts("body_dirt_mapping ok");
    return fails;
}
