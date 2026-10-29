# Speed motes (prototype, default off)

`[video] speed_motes = 1` or `--effect speed_motes=on` adds sparse pale specks that sit still in the world beside the
road ahead of the player car, so they stream past the camera at speed. `speed_motes_amount` (0 to 300, default 100) scales
how many. It is a visual speed cue only.

What it does
- Starts above about 40 percent of the speed the tunnel-vision ramp calls full, and ramps up to full speed.
- Every sim step it may spawn a mote 30 to 70 units ahead of the car and 5 to 16 units to the left or right of its heading,
  0.3 to 3 units above the ground (Track_CameraGroundHeight, falls back to the car height). The central strip is skipped so
  the car and the next bend stay readable.
- Each mote is world-fixed (it never follows the car or camera), lives 40 sim steps and fades in and out. Colour is a pale
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
