#ifndef OPENCMR2_WHATS_NEW_H
#define OPENCMR2_WHATS_NEW_H
// "What's new" changelog: curated, truthful highlights of shipped changes.
// Pure data and counting logic; the screen lives in FrontendWhatsNew.cpp and
// its row at the bottom of the main Options menu in FrontendMenus.cpp (PORT).
// Bump kWhatsNewRevision and add lines when user-visible changes ship; the
// row always lists the whole log, newest last, capped at 18 lines.
inline constexpr int kWhatsNewRevision = 19;

struct WhatsNewLine { int revision; const char *text; };
inline const WhatsNewLine *WhatsNew_Lines(int &count)
{
    static const WhatsNewLine lines[] = {
        {17, "Distance fog with sun glow; drifting air motes, smaller in the cockpit"},
        {1, "God rays: sun shafts through the fog (atmosphere preset)"},
        {1, "Dynamic fog: clear pockets along the route, clearing trend"},
        {1, "Near-field fog clear distance around the car"},
        {1, "Effect amounts and Atmosphere menus; --test-preset atmosphere"},
        {1, "F10 live tuning panel: save, presets, drone controls; overflow fix"},
        {2, "Fog responds to headlights and rear lights"},
        {2, "Snow glints: banks sparkle, flakes twinkle in sun and beams"},
        {2, "Falling snow and air motes respond to light"},
        {7, "Spray, exhaust, splash, backfire, motes, glints overdone"},
        {8, "Rain and snow amount to 300% (default 200%)"},
        {9, "Fix: fog light beams no longer streak the wrong way"},
        {10, "Greece 1 bush and tree cards gain relief with --textures uprez"},
        {11, "Fix: fog lights in bonnet view; cockpit glow gated off the dash"},
        {19, "Windscreen: dirt, dry wipers clear it; broken glass inside"},
        {14, "Fixes: replay chase arrows after the F10 panel; steady replay fog beams"},
        {18, "Tree wind fades with distance; fullscreen startup fix"},
        {16, "What's new moved to the bottom of the Options menu; no popup"},
    };
    count = (int)(sizeof(lines) / sizeof(*lines));
    return lines;
}
// How many lines a user who last saw seenRevision has not been shown yet.
inline int WhatsNew_UnseenCount(int seenRevision)
{
    int n;
    const WhatsNewLine *lines = WhatsNew_Lines(n);
    int unseen = 0;
    for (int i = 0; i < n; i++)
        if (lines[i].revision > seenRevision)
            unseen++;
    return unseen;
}
#endif
