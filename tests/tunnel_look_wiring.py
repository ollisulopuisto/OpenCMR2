import re,sys
h=open('src/render/tunnel_vision.h').read(); sh=open('src/render/shaders/dof.frag').read()
g=open('src/render/gfx_gpu.cpp').read(); e=open('src/render/gl_effects_executor.h').read(); gl=open('src/render/gfx_gl.cpp').read()
ok=True
def need(c,m):
    global ok
    if not c: print("FAIL",m); ok=False
rad=float(re.search(r'kTunnelRadiusPerHeight = ([0-9.]+)f',h).group(1)); inner=float(re.search(r'kTunnelInnerFull = ([0-9.]+)f',h).group(1))
m=re.search(r'float inner = mix\(1\.4, ([0-9.]+), tunnel\);',sh)
need(m is not None and abs(float(m.group(1))-inner)<1e-6,'dof.frag inner radius mirrors kTunnelInnerFull')
need(0.02<=rad<=0.04,'radius stays in the range where 16 taps are not sparse')
need(0.3<=inner<=0.7,'clear circle keeps a car in a chase view (about 0.5 half-heights below centre) sharp')
need('kTunnelRadiusPerHeight * (float)backHeight' in g,'SDL_GPU uses the shared radius')
need('kTunnelRadiusPerHeight * (float)backHeight' in e,'GL uses the shared radius')
need('render/tunnel_vision.h' in g and 'render/tunnel_vision.h' in gl,'both backends include it')
need('0.014f' not in g and '0.014f' not in e,'no stale radius literal')
sys.exit(0 if ok else 1)
