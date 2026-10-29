import re,sys
g=open('src/render/gfx_gpu.cpp').read(); e=open('src/platform/enhancements.h').read(); r=open('game/RallyData.cpp').read()
ok=True
def need(c,m):
    global ok
    if not c: print("FAIL",m); ok=False
need('Sys_GetOption("video.dof_focus_car", 0)' in g,'gated on option, default 0')
need('os.objectId == frame.commands[worldEnd].playerObject' in g,'only the followed car draws')
need(re.search(r'float uniforms\[8\] = \{ focusPos,',g),'uniform x uses resolved focus')
need('{"video.dof_focus_car",1}' in e,'overdo sets it')
need('focus on car' in r,'overlay shows it')
sys.exit(0 if ok else 1)
