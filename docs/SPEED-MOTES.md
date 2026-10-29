# Speed motes (prototype, default off)

`[video] speed_motes = 1` or `--effect speed_motes=on` adds sparse pale specks that sit still in the world beside the
road ahead of the player car, so they stream past the camera at speed. `speed_motes_amount` (0 to 300, default 100) scales
how many. It is a visual speed cue only.

What it does
- Starts above about 40 percent of the speed the tunnel-vision ramp calls full, and ramps up to full speed.
- Every sim step it may spawn a mote ahead of the car and 4 to 14 units to the left or right of its heading, 0.3 to 3.5 units above the ground (Track_CameraGroundHeight, falls back to the car height). The distance ahead is the car's speed per step times 18 to 36 steps, so the car reaches the mote in 0.7 to 1.4 s whatever the world-unit scale (minimum 12 units). The central strip is skipped so
  the car and the next bend stay readable.
- Each mote is world-fixed (it never follows the car or camera), lives 48 sim steps and fades in and out. Colour is a pale
  tan, size is small. It reuses the game's soft smoke texture.
- Budget: at most 64 motes, and it never fills the last 64 slots of the 400-particle pool, so motes never take the last 64 slots. If the pool is full anyway, any other particle spawn takes a mote slot before it evicts an original (Particle_SpawnTagged). Originals can still fill the pool earlier with motes on (about 336 originals plus up to 64 motes) and then evict each other as before; whether that happens was not measured. Purely visual, but it is not 'no effect on original particles' while motes are on.

What it does not do
- No collision object, no physics, handling or road-line input. It reads the car state and writes only particle slots.
- It does not use the global `rand()` stream (own generator). `video.particles` is the rain/snow switch (0 off, 1 rain,
  2 snow); motes do not read it, so `particles=off` leaves motes on. They only follow `speed_motes`.
- It does not change the drone camera or any shake.

Unverified
- Not run natively, no screenshots. Spawn distances assume a world unit of about 1 m (not measured). Lateral 5 to 16 units may
  still be on a wide road or inside scenery; motes can sit in front of trees. Billboard size and alpha are guesses.
- The particle draw path and the sort against other billboards are as the game's own particles; the SDL_GPU and OpenGL paths were not compared.
- Cost was not measured.

Try it: run your usual launch line with `--effect speed_motes=on` and compare with the same line without it (or `=off`), at
high speed on a straight. With `--attract-capture` (see docs/ATTRACT-CAPTURE.md) the AI demo drives; check the speed_raw column
is high enough (above about 0.4 * 1.5 * 65536).

## Revision after the first native run

The first prototype was not identifiable in native screenshots. From source: it spawned 30 to 70 units ahead with a 40-step life, so at speed most motes faded out before the car reached them, and the billboard was about 0.28 units wide (a few pixels at that distance) with at most 35 percent alpha. This revision places each mote by arrival time (18 to 36 steps, life 48), makes the billboard about 0.5 units wide, and raises peak alpha to about 59 percent. These are guesses chosen from that arithmetic, not measured in game.

Diagnostics: while motes are on and the car is above the speed threshold, about every 5 s of simulation the game logs `speed motes: speed_ratio=... spawned=... budget_skipped=... no_ground_height=... live=... billboards_queued=... refused_by_queue=...` plus `billboard queue: peak_depth=N of 800 refused_disabled=N refused_full=N` (spawned, skipped and queued counts are cumulative). `billboards_queued` counts quads that Billboard_Add accepted into the 800-entry per-frame queue, not GPU draws or visible pixels; `refused_by_queue` counts motes it refused (queue disabled or full). Visibility, placement and performance are still unverified natively. The motes are camera-facing billboards of the soft smoke texture, not stretched streaks.

## Optional streak look (video.speed_motes_stretch)

`[video] speed_motes_stretch` (0 to 100, default 0 = round dots, INI only) stretches each mote's billboard along the screen-space direction from the view axis to the mote. A world-fixed point moves outward along that line as the car approaches it, so the stretch reads as passing speed lines. The long axis is 2x the width at rest and up to 9x at full speed at stretch 100 (scaled down linearly below that). The billboard rotation uses the game's own 12-bit angle (derived from Billboard_Draw: the long axis is right*cos(a) + up*sin(a)). Motes closer than 3 units fade out and are full strength from 9 units, so a near streak never fills the screen. Pure maths is covered by tests/speed_mote_streak.cpp; how it looks, and whether the rotation direction matches the renderer on screen, is not verified natively. Streaks reuse the soft smoke texture (a stretched blur, not a hard line). They need `speed_motes=on` and do nothing otherwise.

## Command line, overlay and test preset

Per-launch flags (they win over opencmr2.ini and are never saved): `--speed-fov 0..100`, `--cockpit-eye-shift -100..100`, `--speed-motes-stretch 0..100`, `--speed-motes-amount 0..300`, plus `--effect speed_motes=on`.
`--test-preset overdo` is an exaggerated diagnostic look (motes on at 300% with full streaks, speed FOV 40, DoF on with the largest blur, tunnel vision 100, motion blur on at 60). Any explicit flag overrides a preset value. It is for finding out what the effects do, not a recommended setting, and nothing is saved.
The startup log line `effective speed-feel settings: ...` shows the values the game actually uses. The debug overlay adds `Speed FOV N% (eased speed M%)`, mote state, and `Speed motes live N | queued ok total N (not pixels)` so a missing effect can be told apart from a setting that never reached the game. Native look is unverified.

## Live tuning panel (in a race)

Press F10 during a race to open a panel on the right (`video.tuning_key` in opencmr2.ini changes the key; it is a DirectInput scancode, default 68 = 0x44. A key that is a default driving/menu key, a tuning digit, outside 1..255, or bound to a driving action in your controls is refused and F10 is used, with a log line; on a Mac laptop F10 may need the fn key). Digit keys do the rest, by physical position so they are the same on any layout: `1`/`2` pick a row, `3`/`4` change it (applied at once), `5` resets the row, `6` resets every row to the values they had when the panel was first opened this launch (this includes any frontend menu changes made before the race), `7` saves the shown values to opencmr2.ini. While the panel is open these keys are removed from the keyboard state the game reads, so they do not also drive the car or a menu; F10 never reaches the game. With the panel closed all controls are normal. None of the tuning keys is a default driving key (arrows, space, [ ], C, numpad). A custom binding of one of them to a driving action will stop working while the panel is open.
Until you press `7`, changes are per launch only and opencmr2.ini is untouched; a command-line value can be changed in the panel without editing the INI. Rows: speed FOV, speed motes on/off, mote amount, mote stretch, DoF on/off, DoF blur %, DoF max px (blur radius in pixels when blur is 100%, default 4, also `--dof-blur-max-px`), tunnel vision, motion blur on/off and amount. Visual only. The panel layout and key response have not been checked on screen.
