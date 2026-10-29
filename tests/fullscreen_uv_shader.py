#!/usr/bin/env python3
"""Real generated fullscreen-vertex UV regression. Linux GL is not Metal proof."""
import pathlib, subprocess, tempfile
import moderngl
import numpy as np
root=pathlib.Path(__file__).resolve().parents[1]/'src/render/shaders'
with tempfile.TemporaryDirectory() as d:
 p=d+'/vertex.spv'
 subprocess.run(['glslangValidator','-V',str(root/'motionblur.vert'),'-o',p],check=True)
 vert=subprocess.check_output(['spirv-cross',p,'--version','330','--no-es'],text=True)
ctx=moderngl.create_standalone_context(require=330,backend='egl')
prog=ctx.program(vertex_shader=vert,fragment_shader='''#version 330
in vec2 outTex;out vec4 color;void main(){color=vec4(outTex,0,1);}''')
t=ctx.texture((64,64),4);f=ctx.framebuffer([t]);f.use();ctx.vertex_array(prog,[]).render(vertices=3)
a=np.frombuffer(f.read(components=4),dtype='u1').reshape(64,64,4)
assert a[32,-1,0]>245 and a[32,0,0]<10, 'fullscreen vertex only samples a crop in X'
assert a[-1,32,1]>245 and a[0,32,1]<10, 'fullscreen vertex only samples a crop in Y'
print('PASS actual generated fullscreen vertex spans complete UV range')
