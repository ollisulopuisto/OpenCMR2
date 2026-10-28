"""Actual GPU-kernel pixel test. Synthetic scene, not gameplay/M2 performance."""
from pathlib import Path
import tempfile,subprocess,argparse
import moderngl,numpy as np
from PIL import Image,ImageDraw
parser=argparse.ArgumentParser();parser.add_argument('--output');parser.add_argument('--pool-fixture',type=Path);args=parser.parse_args()
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
def render(yaw,count=3,depth=1.,density=1.,offset=0.,large=False,clouds=None,trail=False,close=False,transmittance=False):
 eye=np.array([np.sin(yaw)*7,2,np.cos(yaw)*7]);eye[0]+=offset;target=np.array([offset,1,0]);forward=(target-eye);forward/=np.linalg.norm(forward);right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward)
 view=np.eye(4);view[:3,:3]=[right,up,forward];view[:3,3]=-view[:3,:3]@eye
 if trail:
  eye=np.array([18+offset,5,55.]) if not close else np.array([20+offset*.3,2.5,28.]);target=np.array([4.,3.,16.]);forward=target-eye;forward/=np.linalg.norm(forward);right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward);view[:3,:3]=[right,up,forward];view[:3,3]=-view[:3,:3]@eye
 # positiveZ D3D-style projection depth0..1, as game shaders consume
 near,far=.1,150 if trail else 30;proj=np.zeros((4,4));proj[0,0]=1.4*h/w;proj[1,1]=1.4;proj[2,2]=far/(far-near);proj[2,3]=-near*far/(far-near);proj[3,2]=1
 inv=np.linalg.inv(proj@view).astype('f4')
 cs=np.zeros((32,4),dtype='f4');cs[:3]=[[-1,1,0,1.4],[0,1.3,.3,1.5],[1.2,1,0,1.3]]
 if large:cs[:3,3]*=3.5
 colors=np.zeros((32,4),dtype='f4');colors[:3]=[.62,.47,.28,.8]
 if clouds is not None:
  count=len(clouds);cs[:count]=clouds[:,:4];colors[:count]=clouds[:,4:]
 data=inv.T.tobytes()+np.array([*eye,count,1/w,1/h,density,far],dtype='f4').tobytes()+cs.tobytes()+colors.tobytes()
 ubo=ctx.buffer(data);ubo.bind_to_uniform_block(0)
 background=pixels.copy();depths=np.full((h,w),depth,dtype='f4')
 if trail:
  yy,xx=np.mgrid[0:h,0:w];clip=np.stack([(xx+.5)/w*2-1,(yy+.5)/h*2-1,np.ones((h,w)),np.ones((h,w))],axis=-1)
  wp=clip@inv.T;wp=wp[...,:3]/wp[...,3,None];ray=wp-eye
  t=-eye[1]/ray[:,:,1];hit=eye+ray*t[:,:,None];valid=(t>0)&(t<1)
  grid=((np.floor(hit[:,:,0]/3)+np.floor(hit[:,:,2]/3))%2).astype(bool)
  background[valid,:3]=np.where(grid[valid,None],[53,68,42],[61,77,49])
  road=(abs(hit[:,:,0]-8*np.sin(hit[:,:,2]*.06))<2)&valid;background[road,:3]=[100,88,65]
  # A 4.2m x 1.8m x 1.5m box is a scale cue, not a game car model.
  lo=np.array([13.,0.,8.]);hi=lo+np.array([1.8,1.5,4.2])
  safe=np.where(abs(ray)<1e-9,1e-9,ray);ta=(lo-eye)/safe;tb=(hi-eye)/safe
  entry=np.max(np.minimum(ta,tb),axis=-1);exit=np.min(np.maximum(ta,tb),axis=-1)
  car=(entry>0)&(entry<exit)&((~valid)|(entry<t))
  hit[car]=eye+ray[car]*entry[car,None];valid|=car;background[car,:3]=[210,216,224]
  hc=np.concatenate([hit,np.ones((h,w,1))],axis=-1)@(proj@view).T
  depths[valid]=(hc[:,:,2]/hc[:,:,3])[valid]
 d=ctx.texture((w,h),1,depths.tobytes(),dtype='f4');d.use(0);prog['sceneDepth'].value=0
 fbo.use();vao.render(vertices=3);raw=np.frombuffer(fbo.read(components=4),dtype='u1').reshape(h,w,4).copy();arr=raw.copy();arr[:,:,:3]=np.clip(raw[:,:,:3].astype(float)+background[:,:,:3].astype(float)*raw[:,:,3,None]/255.,0,255);arr[:,:,3]=pixels[:,:,3];ubo.release();d.release();return raw[:,:,3] if transmittance else arr
assert np.array_equal(render(0,count=0),pixels)
assert np.array_equal(render(0,density=0),pixels)
assert np.array_equal(render(0,depth=0),pixels),'opaque foreground must occlude cloud'
a=render(0);b=render(np.pi/2);
assert not np.array_equal(a,render(0,offset=3.)), 'camera translation must change world-fixed cloud projection'
assert np.count_nonzero(render(0,large=True)[:,:,:3]!=pixels[:,:,:3])>np.count_nonzero(a[:,:,:3]!=pixels[:,:,:3])
assert np.any(a!=pixels) and np.any(b!=pixels)
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
 import json
 poolpath=args.pool_fixture
 if poolpath is not None:
  clouds=np.array(json.loads(poolpath.read_text()),dtype='f4')
  fixed=clouds.copy();trans=render(0,clouds=clouds,trail=True,transmittance=True)
  assert np.min(trans)<51,'large plume must have genuinely opaque core (<20% transmission)'
  assert np.array_equal(clouds,fixed),'rendering must not mutate world anchors'
  close_frames=[]
  for n in [0,8,16,23]:
   im=Image.fromarray(render(0,clouds=clouds,trail=True,close=True,offset=n*.7)).transpose(Image.Transpose.FLIP_TOP_BOTTOM).convert('RGB').resize((720,480))
   ImageDraw.Draw(im).text((8,8),'SYNTHETIC ROAD-LEVEL GPU | fixed world density | white box=4.2m car',fill='white');close_frames.append(im)
  contact=Image.new('RGB',(1440,960))
  for i,im in enumerate(close_frames):contact.paste(im,((i%2)*720,(i//2)*480))
  contact.save(str(Path(args.output).with_name('world-dust-pool-close.png')))
  frames=[]
  for n in range(24):
   arr=render(0,clouds=clouds,trail=True,offset=n*.7)
   im=Image.fromarray(arr).transpose(Image.Transpose.FLIP_TOP_BOTTOM).convert('RGB').resize((720,480))
   ImageDraw.Draw(im).text((8,8),'SYNTHETIC GPU SCENE | actual C++ pool | white box=4.2m car | camera moves right',fill='white');frames.append(im)
  frames[0].save(str(Path(args.output).with_name('world-dust-pool-camera.gif')),save_all=True,append_images=frames[1:],duration=120,loop=0)
  contact=Image.new('RGB',(1440,960))
  for i,n in enumerate([0,7,15,23]):contact.paste(frames[n],((i%2)*720,(i//2)*480))
  contact.save(str(Path(args.output).with_name('world-dust-pool-camera.png')))
 frames=[]
 for n in range(24):
  arr=render(n*np.pi/12,large=True);im=Image.fromarray(arr).convert('RGB').resize((480,320));ImageDraw.Draw(im).text((8,8),f'ACTUAL WORLD DENSITY KERNEL | synthetic larger plume | orbit{n*15}deg',fill='white');frames.append(im)
 frames[0].save(args.output,save_all=True,append_images=frames[1:],duration=100,loop=0)
 frames[0].save(str(Path(args.output).with_suffix('.png')))
 contact=Image.new('RGB',(960,640))
 for i,n in enumerate([0,6,12,18]):contact.paste(frames[n],((i%2)*480,(i//2)*320))
 contact.save(str(Path(args.output).with_name('world-dust-large-contact.png')))
print('PASS GPU dust: translation/orbit, larger coverage, occlusion, deterministic world density, alpha;',ctx.info['GL_RENDERER'])
