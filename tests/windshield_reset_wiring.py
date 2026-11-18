from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'game/Race.cpp').read_text()
n=s.count('Windshield_ResetAll();')
assert n>=2,'Race.cpp must reset windshield on restart and session start (found %d)'%n
# Replay sessions re-accrue damage from the recorded inputs; both replay
# session-start paths must reset the live part table first, gated on the
# replay selection flags so fresh race starts keep their setup damage.
n=s.count('StageTiming_ResetPartDamageForReplay();')
assert n>=2,'Race.cpp must reset part damage on replay session start and replay restart (found %d)'%n
st=(Path(__file__).resolve().parents[1]/'game/StageTiming.cpp').read_text()
fn=st.split('void StageTiming_ResetPartDamageForReplay(void)')[1]
assert 'StageTiming_RebuildDamagedPartMeshes(pCar)' in fn, 'replay reset must restore the exterior appearance (meshes, visibility)'
assert 'CarPartDamage_ClearAccumulated(' in fn, 'replay reset must clear flags AND damage values (accrual re-derives flags from values)'
print('ok windshield reset wiring')
