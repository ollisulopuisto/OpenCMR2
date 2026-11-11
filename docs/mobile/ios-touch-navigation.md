# iOS touch navigation

Implements the approved touch sketch, without changing the existing driving layout.

- A completed single tap inside the game picture skips the current movie.
- In menus, tap accepts the highlighted item, not the row under the finger.
- Finger right-to-left sends Right; left-to-right sends Left.
- Finger top-to-bottom sends Up; bottom-to-top sends Down.
- A stationary two-finger tap sends Escape only after both fingers lift.
- Two-finger swipes, three fingers, holds over 700 ms and ambiguous small drags do nothing.
- One swipe sends one step, on release, using its first qualifying dominant axis.
- Fresh input is required across movie/context/menu transitions. Focus loss resets lost fingers.
- Driving pads are hidden and inactive in iOS menus/movies. Driving gestures are unchanged.
- Android retains its existing Kotlin overlay; desktop keyboard/controller input is unchanged.

The first thresholds are provisional: tap travel is at most 2.5% of the short window edge;
swipe travel starts at 8%; both use physical window proportions, not unequal normalized axes.

## Verification and remaining device trial

Host checks exercise pure gestures and the actual Menu.cpp machine for a language-style
vertical menu, confirm, Back and horizontal slider steps. The existing driving-layout
regression suite still passes. Source syntax checks cover the iOS-gated input path using
SDL 3.4.18 headers, the platform event pump, menu and movie/context source.

These are not an Apple SDK build, a simulator run, or an iPad run. The new app's pixels,
keyboard interaction, tap thresholds, two-finger timing, pause menus and movie-to-menu
carryover still need the real iPad. There are no new on-screen buttons or menu artwork
in this first pass; the sketch's labels describe gestures rather than a replacement menu.

The PR is stacked on the iOS platform-prep branch (#359), which must land first. Preserve
local Xcode Team and bundle ID, and do not delete the app or its Documents/Data during
this trial. Regenerating the project can overwrite manual signing edits.
