#ifndef OPENCMR2_WHATS_NEW_H
#define OPENCMR2_WHATS_NEW_H
// "What's new" changelog: curated, truthful highlights of shipped changes.
// Pure data and counting logic; the screen and its wiring live in
// FrontendScreens.cpp (PORT). Bump kWhatsNewRevision and add lines when
// user-visible changes ship: the screen then shows once on the main menu
// (tracked in ui.whatsnew_seen) and stays reachable from the Screen page.
inline constexpr int kWhatsNewRevision = 6;

struct WhatsNewLine { int revision; const char *text; };
inline const WhatsNewLine *WhatsNew_Lines(int &count)
{
    static const WhatsNewLine lines[] = {
        {1, "Distance fog with sun glow and drifting air motes"},
        {1, "God rays: sun shafts through the fog (atmosphere preset)"},
        {1, "Dynamic fog: clear pockets along the route, clearing trend"},
        {1, "Near-field fog clear distance around the car"},
        {1, "Effect amounts and Atmosphere menus under Graphics options"},
        {1, "F10 live tuning panel: save, presets, drone controls"},
        {1, "rebuild.sh --test-preset atmosphere for the full look"},
        {1, "Smaller, livelier air motes from native feedback"},
        {1, "Fix: tuning panel row overflow crash"},
        {2, "Fog responds to headlights and rear lights"},
        {2, "Snow glints: banks sparkle, flakes twinkle in sun and beams"},
        {2, "Falling snow and air motes respond to light"},
        {2, "Snow ground lighting prototype (off: --snow-material 100 to try)"},
        {2, "Drone tuning: spring down to 5%, height drop up to 200%"},
        {3, "Atmosphere menu directly under Graphics options"},
        {4, "Drone rails widen: distance 300%, height/spring 400%, drop 500%"},
        {5, "Tyre debris to 2000%, dust/snow plume to 300% - overdone, dial down"},
        {6, "Wheel spray and exhaust smoke to 300% (default 200%)"},
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
