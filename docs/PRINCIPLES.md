# Project principle: add to the audio-visuals and feel, never change physics or gameplay

OpenCMR2 does not change the physics or the gameplay of Colin McRae Rally 2.0.
It adds to the audio-visuals and to the feel. The original vehicle code, handling,
collisions, damage, stage layout, racing lines, timing, rules and controls stay as
they are, and the physics regression tests ([PHYSICS_PARITY.md](PHYSICS_PARITY.md))
must keep passing bit for bit.

This is the owner's rule for the project. It applies to every enhancement, setting,
mod and experiment, and it steers all future work.

## What it allows

- Graphics: lighting, shadows, effects, textures, scenery that looks different, post
  processing, anti-aliasing and upscaling.
- Audio.
- Camera and presentation: extra camera modes and framing that sit next to the
  original ones. Original cameras, controls and profiles are never removed or
  replaced.
- Anything that only reads simulation state (speed, position) to drive a visual.
  Speed-driven drone distance and tunnel vision are examples.

## What it rules out

- Changing how the car drives, collides, takes damage, or how stages, rules and
  timing work.
- Replacing an original option or mode instead of adding a new one.

## Named exceptions (set by the owner)

1. **Curved roads.** Allowed as a visual upgrade: smoother-looking roads. The drive
   line must stay the same: same racing line, same handling. It is not permission
   to redesign bends. Any road change has to be checked against the original line.
   Matching the render and the contact surface to each other is not enough on its
   own, because the contact surface is what the car drives on.
2. **Breakable trees.** An exception to the gameplay rule, at low priority. Not
   started. It needs an explicit source-backed design (see
   [TREE-COLLISION.md](TREE-COLLISION.md)) before any behaviour changes.

No other exception exists. In particular a car or mod that changes handling needs
its own explicit decision from the owner. Settings and saves from earlier
experiments stay as they are until the owner decides what to do with them.

## Status of earlier work against this rule

See the audit notes in the pull request that added this file. Known open item:
`--roads curved` is an experiment that changes render and contact geometry
together (see [CURVED-ROAD-RESEARCH.md](CURVED-ROAD-RESEARCH.md): it moved sampled
surface heights by up to about 12.7 cm), so it is not yet shown to keep the drive
line.
