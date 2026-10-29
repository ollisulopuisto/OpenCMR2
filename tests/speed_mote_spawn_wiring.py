import sys
c=open('game/CarEffects.cpp').read(); h=open('game/DustPlume.h').read()
i=c.index('void SpeedMotes_Step'); body=c[i:i+3500]
ok=True
def need(x,m):
    global ok
    if not x: print('FAIL',m); ok=False
need('car->right.x' in body and 'car->right.z' in body,'heading comes from Car::right (the driving axis)')
need('car->forward' not in body,'Car::forward (the side axis) is not used as the heading')
need('SpeedMote_SpawnOffset(hx, hz' in body,'spawn uses the tested helper')
need('velocity . right >= 0' in h,'the axis evidence stays documented next to the helper')
sys.exit(0 if ok else 1)
