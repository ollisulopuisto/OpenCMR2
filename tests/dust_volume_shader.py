"""Actual GPU-kernel pixel test. Synthetic scene, not gameplay/M2 performance."""
from pathlib import Path
import tempfile,subprocess,argparse
import moderngl,numpy as np
from PIL import Image,ImageDraw
parser=argparse.ArgumentParser();parser.add_argument('--output');args=parser.parse_args()
root=Path(__file__).resolve().parents[1]/'src/render/shaders'
with tempfile.TemporaryDirectory() as tmp:
 spv=str(Path(tmp)/'v.spv');subprocess.run(['glslangValidator','-V',str(root/'dust_volume.frag'),'-o',spv],check=True)
 frag=subprocess.check_output(['spirv-cross',spv,'--version','330','--no-es'],text=True)
ctx=moderngl.create_standalone_context(require=330,backend='egl');w,h=240,160
vert='''#version330
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);}'''.replace('#version330','#version 330')
prog=ctx.program(vertex_shader=vert,fragment_shader=frag);prog['DustVolumeUniforms'].binding=0
pixels=np.zeros((h,w,4),dtype='u1');pixels[:]=[45,70,53,255]
scene=ctx.texture((w,h),4,pixels.tobytes());scene.use(0)
out=ctx.texture((w,h),4);fbo=ctx.framebuffer([out]);vao=ctx.vertex_array(prog,[])
def render(yaw,count=3,depth=1.,density=1.):
 eye=np.array([np.sin(yaw)*7,2,np.cos(yaw)*7]);target=np.array([0,1,0]);forward=(target-eye);forward/=np.linalg.norm(forward);right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward)
 view=np.eye(4);view[:3,:3]=[right,up,forward];view[:3,3]=-view[:3,:3]@eye
 # positiveZ D3D-style projection depth0..1, as game shaders consume
 near,far=.1,30;proj=np.zeros((4,4));proj[0,0]=1.4*h/w;proj[1,1]=1.4;proj[2,2]=far/(far-near);proj[2,3]=-near*far/(far-near);proj[3,2]=1
 inv=np.linalg.inv(proj@view).astype('f4')
 cs=np.zeros((16,4),dtype='f4');cs[:3]=[[-1,1,0,1.4],[0,1.3,.3,1.5],[1.2,1,0,1.3]]
 colors=np.zeros((16,4),dtype='f4');colors[:3]=[.62,.47,.28,.8]
 data=inv.T.tobytes()+np.array([*eye,count,1/w,1/h,density,30],dtype='f4').tobytes()+cs.tobytes()+colors.tobytes()
 ubo=ctx.buffer(data);ubo.bind_to_uniform_block(0)
 d=ctx.texture((w,h),1,np.full((h,w),depth,dtype='f4').tobytes(),dtype='f4');d.use(0);prog['sceneDepth'].value=0
 fbo.use();vao.render(vertices=3);raw=np.frombuffer(fbo.read(components=4),dtype='u1').reshape(h,w,4).copy();arr=raw.copy();arr[:,:,:3]=np.clip(raw[:,:,:3].astype(float)+pixels[:,:,:3].astype(float)*raw[:,:,3,None]/255.,0,255);arr[:,:,3]=pixels[:,:,3];ubo.release();d.release();return arr
assert np.array_equal(render(0,count=0),pixels)
assert np.array_equal(render(0,density=0),pixels)
assert np.array_equal(render(0,depth=0),pixels),'opaque foreground must occlude cloud'
a=render(0);b=render(np.pi/2);assert np.any(a!=pixels) and np.any(b!=pixels)
assert np.array_equal(a,render(0)),'same world/camera must render deterministically'
assert np.array_equal(a[:,:,3],pixels[:,:,3])
# Exercise actual depth-aware composite separately, not a CPU substitute.
with tempfile.TemporaryDirectory() as tmp:
 spv=str(Path(tmp)/'c.spv');subprocess.run(['glslangValidator','-V',str(root/'dust_composite.frag'),'-o',spv],check=True)
 cf=subprocess.check_output(['spirv-cross',spv,'--version','330','--no-es'],text=True)
cp=ctx.program(vertex_shader=vert,fragment_shader=cf);cp['DustCompositeUniforms'].binding=0
cu=ctx.buffer(np.array([1/w,1/h,0,0],dtype='f4').tobytes());cu.bind_to_uniform_block(0)
scene.use(0);cp['scene'].value=0
clear=np.zeros((h//2,w//2,4),dtype='u1');clear[:,:,3]=255
volume=ctx.texture((w//2,h//2),4,clear.tobytes());volume.filter=(moderngl.NEAREST,moderngl.NEAREST);volume.use(1);cp['volume'].value=1
d=ctx.texture((w,h),1,np.ones((h,w),dtype='f4').tobytes(),dtype='f4');d.use(2);cp['sceneDepth'].value=2
fbo.use();ctx.vertex_array(cp,[]).render(vertices=3)
neutral=np.frombuffer(fbo.read(components=4),dtype='u1').reshape(h,w,4)
assert np.array_equal(neutral,pixels),'clear half-res cloud must preserve full-res scene'
if args.output:
 frames=[]
 for n in range(24):
  arr=render(n*np.pi/12);im=Image.fromarray(arr).convert('RGB').resize((480,320));ImageDraw.Draw(im).text((8,8),f'ACTUAL GPU DENSITY KERNEL | synthetic scene | orbit{n*15}deg',fill='white');frames.append(im)
 frames[0].save(args.output,save_all=True,append_images=frames[1:],duration=100,loop=0)
 frames[0].save(str(Path(args.output).with_suffix('.png')))
print('PASS GPU dust volume: no-op/zero density, foreground occlusion, camera orbit, deterministic world density, alpha;',ctx.info['GL_RENDERER'])
