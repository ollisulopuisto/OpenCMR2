# Optional real-driver names

This source-only patch writes a separate `driver-names.tsv`. It does not modify
an original EXE, BFL, game configuration or any other original game file.
The installer only checks that the input is an installed PC game directory.
No graphics pack or Python package dependencies are needed. Use Python 3.10+.

From the source checkout, replace the input path with your owned game folder:

```sh
python3 tools/driver-name-patch/install.py --input-dir "/path/to/owned/CMR2" --user-dir "$HOME/OpenCMR2-private"
./build/macos/src/opencmr2 --data "/path/to/owned/CMR2" --user-dir "$HOME/OpenCMR2-private"
```

The input must contain Game, FrontEnd, Sounds, CountrySpecific and SetupRep.
The user directory must be outside that input tree. Existing driver-name files
(including symlinks) are never replaced. It can share the separate private
profile used for scenery assets. A new profile has separate settings/saves/logs;
existing saves are not copied. Generated names are loaded at first name lookup,
so restart after installing or removing the sidecar.

## Undo and fallback

Move or remove only `driver-names.tsv` from that private profile, then restart.
Alternatively set `driver_name_overrides=0` in its existing `[video]` INI section.
Missing, oversized or malformed files retain the original names. The file uses
a version header and exactly 15 ordered slot/name pairs; validation completes
before any names apply. No car, driver skill, AI, performance or physics values
change. Other names embedded in artwork or recordings are not replaced.
The game may write saves/settings/logs in its chosen user directory, but neither
this installer nor the runtime name loader writes the original input files.

## Mapping provenance

The 15-slot mapping is the same one previously imported into this fork from
the supplied `CMR2_real_driver_names_patch_PC.zip / CMR2.exe` static name table.
The executable was inspected as data, never executed or redistributed. Its
SHA256 is `87c235b054853480559b2d7902a6a5c0021c22838b44ef2beeaefe8a46c458e3`;
table offset `0x1166b4`, 15 entries, 20-byte stride. This is that patch's slot
mapping, not a new claim about a particular championship season. ASCII initials
and spellings are retained:

| Slot | Name |
| --- | --- |
| 0 | t. makinen |
| 1 | c. mcrae |
| 2 | r. burns |
| 3 | d. auriol |
| 4 | c. sainz |
| 5 | j. kankkunen |
| 6 | m. gronholm |
| 7 | f. loix |
| 8 | f. delecour |
| 9 | h. rovanpera |
| 10 | t. radstrom |
| 11 | t. gardemeister |
| 12 | p. liatti |
| 13 | p. solberg |
| 14 | b. thiry |

The installer emits only this short text table, not proprietary binary data.
The runtime parser is portable; injected runtime I/O tests use GNU linker
wrapping on Linux. Native Mac menu appearance remains unverified.

```sh
python3 tools/driver-name-patch/test_install.py
```
