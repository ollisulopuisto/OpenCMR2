# Attract-demo capture for A/B comparisons

`--attract-capture DIR` launches into the attract demo (it skips the 30.5 s idle wait on the main menu), saves PNGs of the presented frame (HUD and debug overlay included) at fixed **stage-clock** times, writes `capture.csv`, and quits.

```
--attract-capture DIR     output folder (must exist)
--capture-interval SEC    stage-clock seconds between shots, 1..60, default 5
--capture-count N         number of shots, 1..120, default 7 (= 0, 5 ... 30 s)
```

Files: `DIR/demo-t000.png`, `demo-t005.png`, ... and `capture.csv` with `target_s,stage_clock_ms,country,stage,speed_raw` for each shot.

Why stage clock: the shot is requested on the first rendered frame at or after the target clock value, so both runs of an A/B pair line up on the simulation clock, not on wall time. The renderer saves the next presented frame, so the true clock is the logged value and can be a frame or so later than the target.

## A/B example

Run it twice with the same flags except the effect under test, into two folders:

```
./build/macos/src/opencmr2 --data "/Users/dst/Documents/games/CMR2" <your usual flags> \
  --effect depth_of_field=off --attract-capture /tmp/ab/dof-off --capture-interval 5 --capture-count 7
./build/macos/src/opencmr2 --data "/Users/dst/Documents/games/CMR2" <your usual flags> \
  --effect depth_of_field=on  --attract-capture /tmp/ab/dof-on  --capture-interval 5 --capture-count 7
```

Then compare `capture.csv` first: `country`, `stage` must match in both, and `stage_clock_ms` and `speed_raw` should be close. If they differ, the demo did not repeat and the pair is not comparable. The demo setup rotates through four fixed setups per launch and starts at the first; the AI drive is not verified frame-for-frame repeatable.

## Limits

- Capture only: it reads the stage clock, the stage and the player car speed. It changes no gameplay or physics input.
- The demo runs the AI attract drive and the demo camera; the drone camera knobs only apply if the demo uses the drone camera.
- If the demo ends before all shots, the log says how many were taken and the game quits.
- Not run natively by the author; the first real captures come from a native run. On the OpenGL backend the frame readback is the existing frame-dump path and its row order is unverified.
