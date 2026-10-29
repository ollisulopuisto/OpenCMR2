# Attract-demo capture for A/B comparisons

`--attract-capture DIR` launches into the attract demo (it skips the 30.5 s idle wait on the main menu), saves PNGs of the presented frame (HUD and debug overlay included) at fixed **stage-clock** times, writes `capture.csv`, and quits.

```
--attract-capture DIR     output folder (must exist)
--capture-interval SEC    stage-clock seconds between shots, 1..60, default 5
--capture-count N         number of shots, 1..120, default 7 (= 0, 5 ... 30 s)
```

Files: `DIR/demo-t000.png`, `demo-t005.png`, ... and `capture.csv` with one row per shot:

`target_s, request_clock_ms, saved_clock_ms, tick_frames_to_save, country, stage, speed_raw, car_x, car_y, car_z, view_type, drone_view`

The state columns (`country`..`drone_view`, raw fixed-point car position, active camera record type and whether it is the drone variant) are sampled when the capture is requested. The renderer saves the next presented frame; `saved_clock_ms` is the stage clock when the save was seen and `tick_frames_to_save` the number of update ticks it took (1 means the frame rendered right after the request). A larger gap means the saved picture can be a few frames later than the logged state. The row is the state signature of the shot, not a camera pose: the camera position itself is not logged.

Why stage clock: the shot is requested on the first rendered frame at or after the target clock value, so both runs of an A/B pair line up on the simulation clock, not on wall time. The renderer saves the next presented frame, so the true clock is the logged value and can be a frame or so later than the target.

## A/B example

Run it twice with the same flags except the effect under test, into two folders:

```
./build/macos/src/opencmr2 --data "/Users/dst/Documents/games/CMR2" <your usual flags> \
  --effect depth_of_field=off --attract-capture /tmp/ab/dof-off --capture-interval 5 --capture-count 7
./build/macos/src/opencmr2 --data "/Users/dst/Documents/games/CMR2" <your usual flags> \
  --effect depth_of_field=on  --attract-capture /tmp/ab/dof-on  --capture-interval 5 --capture-count 7
```

Then compare the two `capture.csv` files row by row before looking at pictures. `country`, `stage`, `view_type` and `drone_view` must be identical, and `car_x/y/z`, `speed_raw` and the clock columns should be close. Close time and speed alone do not make a pair comparable: the screenshots from a native run showed the demo camera can differ between shots. If any row differs, treat that pair as not comparable. The demo setup rotates through four fixed setups per launch and starts at the first; the AI drive is not verified frame-for-frame repeatable.

## Limits

- Capture only: it reads the stage clock, the stage and the player car speed. It changes no gameplay or physics input.
- The demo runs the AI attract drive and the demo camera; the drone camera knobs only apply if the demo uses the drone camera.
- If the demo ends before all shots, the log says how many were taken and the game quits.
- Not run natively by the author; the first real captures come from a native run. On the OpenGL backend the frame readback is the existing frame-dump path and its row order is unverified.
