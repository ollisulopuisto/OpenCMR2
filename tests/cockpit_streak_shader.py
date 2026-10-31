#!/usr/bin/env python3
"""Optional real shader pixels on synthetic depths. No game assets or native claim."""
from pathlib import Path
import subprocess,tempfile
import moderngl,numpy as np
r=Path(__file__).resolve().parents[1];W,H=192,128;near,far=.1,150.
with tempfile.TemporaryDirectory() as t:
 spv=str(Path(t)/'shader.spv');subprocess.run(['glslangValidator','-V',str(r/'src/render/shaders/speedstreak.frag'),'-o',spv],check=True,stdout=subprocess.DEVNULL);frag=subprocess.check_output(['spirv-cross',spv,'--version','330','--no-es'],text=True)
c=moderngl.create_standalone_context(require=330,backend='egl');p=c.program(vertex_shader='#version 330\nvoid main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);}',fragment_shader=frag);p['SpeedStreakUniforms'].binding=0;p['scene']=0;p['depthTex']=1
src=np.zeros((H,W,4),np.uint8);src[:,:,3]=255;src[:,::2,:3]=255;scene=c.texture((W,H),4,src.tobytes());scene.filter=(moderngl.LINEAR,moderngl.LINEAR);scene.repeat_x=scene.repeat_y=False;scene.use(0)
def run(z,clear=1.6,n=near,f=far,strength=1.,lines=0.):
 depth=np.full((H,W),far/(far-near)*(1-near/z),np.float32);d=c.texture((W,H),1,depth.tobytes(),dtype='f4');d.filter=(moderngl.NEAREST,moderngl.NEAREST);d.use(1);u=np.zeros(256,np.float32);u[:4]=[strength,W/H,1,lines];u[248:252]=[clear,n,f,0];u[252:]=[1.15,1,0,0];b=c.buffer(u.tobytes());b.bind_to_uniform_block(0);target=c.texture((W,H),4);out=c.framebuffer([target]);out.use();vao=c.vertex_array(p,[]);vao.render(vertices=3);pixels=np.frombuffer(out.read(components=4),np.uint8).reshape(H,W,4).copy()
 for o in [d,b,vao,out,target]:o.release()
 return pixels
assert np.array_equal(run(.5),src),'near cockpit must be sharp'
assert np.array_equal(run(.5,lines=1),src),'near cockpit must exclude lines too'
assert np.array_equal(run(20),run(20,clear=0)),'distant speed feel must stay identical'
assert np.array_equal(run(.5,n=0,f=0),run(.5,clear=0)),'unknown depth must retain old look'
assert np.array_equal(run(.5,n=1,f=.5),run(.5,clear=0)),'invalid projection must retain old look'
assert np.array_equal(run(.5,strength=0),src),'stopped foreground unchanged'
assert np.abs(run(.5,clear=0).astype(float)-src).mean()>5,'probe must expose unprotected smear'
print('translated cockpit streak shader pixels: sharp near surfaces and lines, unchanged distant/unknown/invalid/stopped: pass')
