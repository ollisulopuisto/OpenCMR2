# OpenCMR2 roadmap

The living list of the visual upgrade program. Ground rules for everything on
this page: never change physics or gameplay, never edit the original game
files (every change is a load-time mod behind its own flag), and every effect
must work on Metal. Look changes go through visual-test previews before they
land.

## Vegetation: everything 3D

Done so far:

- Tree volume shells on converted models: foliage-only UV plus a fixed 3D
  trunk; the swirl seen from below is fixed (#385).
- Bush extrusion, enabled by `--all-mods` (#295).
- Finland shallow joined-strip relief with wind (PR185).
- France1 hedge/treewall relief with wind, original triangle domains
  preserved (PR186).

Next, in order:

1. **Finland1 remaining 2D vegetation.** Hedge rows, brush and forest strips
   still render as flat cards next to the converted trees. Goal: no flat
   cards left in Finland1.
2. **Greece1 roadside bushes.** Extrude the razor-thin 2D bushes away from
   the road: the road-facing edge stays fixed so driving room and collision
   are unchanged. Blocked on the Greece BFL source export from the game
   install.
3. **Remaining stages.** Continue the tree/bush conversion (Sweden is in
   progress), then the rest of the tracks.
4. **Tree look fixes.** Some converted crowns read too wide and spotted;
   upscale the tree textures alongside.

## Track geometry

- Curved-geometry rounding on all tracks. Finland1 and Greece1 world
  geometry landed (PR184, PR188 to PR189); the rest of the stages follow the
  same per-stage pass until all track geometry is covered.

## Spectators

- More animation for the 2D crowd: heads tracking the car, hands up and
  jumping on fast passes. 2D people stay 2D; volume is for vegetation only.

## How work lands

- One PR per item, binary flag per enhancement, preview or visual tests
  first for anything that changes the look, CI plus review gates before
  merge, and a note per item of what is verified on a native Mac versus only
  in CI.
