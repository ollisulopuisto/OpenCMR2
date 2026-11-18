from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'game/Race.cpp').read_text()
n=s.count('Windshield_ResetAll();')
assert n>=2,'Race.cpp must reset windshield on restart and session start (found %d)'%n
# Replay playback re-accrues damage from the recorded inputs; the live part
# table must reset at the real playback boundary - Replay_SelectAndInitializeLane
# setting playing=1, which a live session never crosses. It must NOT sit in the
# pre-race/restart transitions gated on mode or selection flags: game mode 4 is
# also live knockout, and flags 26/27 are race configuration (modes 5/6, stage
# 10), so those gates erase live carryover and miss actual replays.
assert 'StageTiming_ResetPartDamageForReplay();' not in s, 'replay damage reset must not run in Race.cpp session transitions'
so=(Path(__file__).resolve().parents[1]/'game/StageObjects.cpp').read_text()
fn=so.split('int Replay_SelectAndInitializeLane(ReplayStream *p, short lane, short start, BYTE car)')[1]
assert 'StageTiming_ResetPartDamageForReplay();' in fn, 'replay damage reset must run when playback starts (playing=1)'
assert fn.index('StageTiming_ResetPartDamageForReplay();') < fn.index('p->playing = 1;'), 'reset must precede the playback start'
st=(Path(__file__).resolve().parents[1]/'game/StageTiming.cpp').read_text()
fn=st.split('void StageTiming_ResetPartDamageForReplay(void)')[1]
assert 'StageTiming_RebuildDamagedPartMeshes(pCar)' in fn, 'replay reset must restore the exterior appearance (meshes, visibility)'
assert 'CarPartDamage_ClearAccumulated(' in fn, 'replay reset must clear flags AND damage values (accrual re-derives flags from values)'
print('ok windshield reset wiring')
