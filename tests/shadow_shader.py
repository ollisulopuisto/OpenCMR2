#!/usr/bin/env python3
"""Actual fixed.frag pixels: shadow coordinates/depth and restrained contrast.
Synthetic inputs, not game footage or native SDL/Metal validation.
"""
import pathlib, subprocess, tempfile, argparse
import moderngl
import numpy as np
from PIL import Image, ImageDraw
p=argparse.ArgumentParser();p.add_argument('--output');args=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
ctx=moderngl.create_standalone_context(require=330,backend='egl')
with tempfile.TemporaryDirectory() as d:
    spv=d+'/f.spv'
    subprocess.run(['glslangValidator','-V',str(root/'src/render/shaders/fixed.frag'),'-o',spv],check=True)
    fs=subprocess.check_output(['spirv-cross',spv,'--version','330','--no-es'],text=True)
vs='''#version 330
out vec4 inDiffuse; out vec4 inSpecular; out vec4 inTex0; out vec4 inTex1;
out float inFog; out float inEyeDepth; out vec3 inEyePosition; out vec4 inLightPosition;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2-1,0,1);
inDiffuse=vec4(.7,.8,.6,1);inSpecular=vec4(0);inTex0=inTex1=vec4(p,0,1);
inFog=1;inEyeDepth=1;inEyePosition=vec3(0);inLightPosition=vec4(p*2-1,.6,1);}
'''
program=ctx.program(vertex_shader=vs,fragment_shader=fs)
# std140 blocks. Integer fields are written as integers.
size=program['FragmentUniforms'].size
u=np.zeros(size//4,dtype='u4');f=u.view('f4')
u[0:4]=[2,0,0,2];u[8:12]=[1,0,0,1] # diffuse stage, stage1 disabled
# alpha test disabled, fog disabled, no distance fade; shadow vec4 followed by zero body-dirt vec4.
f[-8:-4]=[0.0005,1,0,0]
ubo=ctx.buffer(u.tobytes());program['FragmentUniforms'].binding=0;ubo.bind_to_uniform_block(0)
for name in ['tex0','tex1','backdrop','native0','native1']:
    if name in program: program[name].value=0
tex=ctx.texture((1,1),4,bytes([255]*4));tex.use(0)
cube=ctx.texture_cube((1,1),4,bytes([255]*24));cube.use(1)
for name in ['cube0','cube1']:
    if name in program: program[name].value=1
program['shadowMap'].value=2
# Top half shallow occluder, bottom half clear. Use framebuffer orientation
# explicitly: flipped y lookup must shadow the bottom image-space half here.
depth=np.ones((64,64),dtype='f4');depth[:32,:32]=.3
shadow=ctx.texture((64,64),1,depth.tobytes(),dtype='f4');shadow.filter=(moderngl.NEAREST,moderngl.NEAREST);shadow.repeat_x=shadow.repeat_y=False;shadow.use(2)
target=ctx.texture((128,128),4);fb=ctx.framebuffer([target]);fb.use();vao=ctx.vertex_array(program,[])
def render(enabled):
    f[-7]=enabled;ubo.write(u.tobytes());vao.render(vertices=3)
    return np.frombuffer(fb.read(components=4),dtype='u1').reshape(128,128,4).copy()
off=render(0);on=render(1)
assert np.array_equal(on[16:48,16:48],off[16:48,16:48]), 'clear region darkened: y flip/depth wrong'
ratio=on[80:112,16:48,:3].astype(float)/off[80:112,16:48,:3]
assert np.all((ratio>.53)&(ratio<.57)), 'occluded region missing or crushed to black'
assert np.array_equal(on[:,:,3],off[:,:,3]), 'shadow changed alpha'
if args.output:
    image=Image.new('RGB',(512,300),'#202329');draw=ImageDraw.Draw(image)
    for i,(label,pixels) in enumerate([('OFF',off),('ON: AMBIENT RETAINED',on)]):
        image.paste(Image.fromarray(pixels[::-1]).resize((256,256)),(i*256,24));draw.text((i*256+8,5),label,fill='white')
    draw.text((8,284),'SYNTHETIC FIXED-SHADER PIXELS, NOT GAME FOOTAGE',fill='white');image.save(args.output)
print('shadow shader: depth convention, Y direction, contrast and alpha pass')
