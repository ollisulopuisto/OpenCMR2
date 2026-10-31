#ifndef OPENCMR2_WHATS_NEW_H
#define OPENCMR2_WHATS_NEW_H
// "What's new" changelog: curated, truthful highlights of shipped changes.
// Pure data and counting logic; the screen and its wiring live in
// FrontendScreens.cpp (PORT). Bump kWhatsNewRevision and add lines when
// user-visible changes ship: the screen then shows once on the main menu
// (tracked in ui.whatsnew_seen) and stays reachable from the Screen page.
inline constexpr int kWhatsNewRevision = 14;

struct WhatsNewLine { int revision; const char *text; };
inline const WhatsNewLine *WhatsNew_Lines(int &count)
{
    static const WhatsNewLine lines[] = {
        {1, "Distance fog with sun glow; smaller, livelier drifting air motes"},
        {1, "God rays: sun shafts through the fog (atmosphere preset)"},
        {1, "Dynamic fog: clear pockets along the route, clearing trend"},
        {1, "Near-field fog clear distance around the car"},
        {1, "Effect amounts and Atmosphere menus; --test-preset atmosphere"},
        {1, "F10 live tuning panel: save, presets, drone controls; overflow fix"},
        {2, "Fog responds to headlights and rear lights"},
        {2, "Snow glints: banks sparkle, flakes twinkle in sun and beams"},
        {2, "Falling snow and air motes respond to light"},
        {5, "Tyre debris to 2000%, dust/snow plume to 300% - overdone, dial down"},
        {7, "Spray, exhaust, splash, backfire, motes, glints overdone"},
        {8, "Rain and snow amount to 300% (default 200%)"},
        {9, "Fix: fog light beams no longer streak the wrong way"},
        {10, "Greece 1 bush and tree cards gain relief with --textures uprez"},
        {11, "Fix: fog lights in bonnet view; cockpit glow gated off the dash"},
        {12, "Windscreen: dirt builds on the glass, wipers clear the sweep"},
        {13, "Fix: replay chase arrows work after using the F10 panel."},
        {14, "Fix: steady fog light beams in replays; glow sits ahead of car"},
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
