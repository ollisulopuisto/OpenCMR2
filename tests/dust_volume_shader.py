"""Actual GPU-kernel pixel test. Synthetic scene, not gameplay/M2 performance."""
from pathlib import Path
import tempfile,subprocess,argparse
import moderngl,numpy as np
from PIL import Image,ImageDraw
parser=argparse.ArgumentParser();parser.add_argument('--output');parser.add_argument('--benchmark',action='store_true',help='synthetic draw timing only, not gameplay performance');parser.add_argument('--pool-fixture',type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[1]/'src/render/shaders'
def compile_volume(source):
 with tempfile.TemporaryDirectory() as tmp:
  path=Path(tmp)/'volume.frag';path.write_text(source)
  spv=str(Path(tmp)/'v.spv');subprocess.run(['glslangValidator','-V',str(path),'-o',spv],check=True)
  return subprocess.check_output(['spirv-cross',spv,'--version','330','--no-es'],text=True)
source=(root/'dust_volume.frag').read_text()
frag=compile_volume(source)
ctx=moderngl.create_standalone_context(require=330,backend='egl');w,h=240,160
# Emulate SDL GPU's TOP-origin fragment position in bottom-origin EGL. Depth
# uploads below are flipped too. Previous tests used GL convention on both
# sides and therefore could not catch a Metal/Vulkan clip-Y mismatch.
frag=frag.replace('gl_FragCoord.xy', 'vec2(gl_FragCoord.x, '+str(h)+'.0 - gl_FragCoord.y)')

vert='''#version330
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);}'''.replace('#version330','#version 330')
prog=ctx.program(vertex_shader=vert,fragment_shader=frag);prog['DustVolumeUniforms'].binding=0
pixels=np.zeros((h,w,4),dtype='u1');pixels[:]=[45,70,53,255]
scene=ctx.texture((w,h),4,pixels.tobytes());scene.use(0)
out=ctx.texture((w,h),4);fbo=ctx.framebuffer([out]);vao=ctx.vertex_array(prog,[])
def render(yaw,count=3,depth=1.,density=1.,offset=0.,large=False,clouds=None,trail=False,close=False,transmittance=False,pitch=0.,gpu_times=None):
 eye=np.array([np.sin(yaw)*7,2,np.cos(yaw)*7]);eye[0]+=offset;target=np.array([offset,1,0]);forward=(target-eye);forward/=np.linalg.norm(forward);right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward)
 view=np.eye(4);view[:3,:3]=[right,up,forward];view[:3,3]=-view[:3,:3]@eye
 if trail:
  eye=np.array([18+offset,5,55.]) if not close else np.array([20+offset*.3,2.5,28.]);target=np.array([4.,3.,16.]);forward=target-eye;forward/=np.linalg.norm(forward);right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward);view[:3,:3]=[right,up,forward];view[:3,3]=-view[:3,:3]@eye
 if pitch:
  eye[1]+=pitch;forward=target-eye;forward/=np.linalg.norm(forward);right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward);view[:3,:3]=[right,up,forward];view[:3,3]=-view[:3,:3]@eye
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
 d=ctx.texture((w,h),1,np.flipud(depths).copy().tobytes(),dtype='f4');d.use(0);prog['sceneDepth'].value=0
 fbo.use()
 if gpu_times is None:vao.render(vertices=3)
 else:
  query=ctx.query(time=True)
  with query:vao.render(vertices=3)
  gpu_times.append(query.elapsed/1e6)
 raw=np.frombuffer(fbo.read(components=4),dtype='u1').reshape(h,w,4).copy();arr=raw.copy();arr[:,:,:3]=np.clip(raw[:,:,:3].astype(float)+background[:,:,:3].astype(float)*raw[:,:,3,None]/255.,0,255);arr[:,:,3]=pixels[:,:,3];ubo.release();d.release();return raw[:,:,3] if transmittance else arr
# Compare the real optimized kernel with the previous per-lobe evaluation.
# Reconstruct just that expression site, not an unrelated CPU approximation.
shared='float detail=noise(p*1.7);'
assert source.count(shared)==1
reference=source.replace(shared,'').replace('float d=fall*',shared+'\n            float d=fall*')
reference_frag=compile_volume(reference).replace('gl_FragCoord.xy', 'vec2(gl_FragCoord.x, '+str(h)+'.0 - gl_FragCoord.y)')
reference_prog=ctx.program(vertex_shader=vert,fragment_shader=reference_frag)
reference_prog['DustVolumeUniforms'].binding=0
reference_vao=ctx.vertex_array(reference_prog,[])
optimized_prog,optimized_vao=prog,vao
# Unlike count=32 with only three nonzero entries, this is a genuine dense
# 32-lobe overlap. It exercises the repeated detail-noise site under load.
clouds=np.zeros((32,8),dtype='f4')
for i in range(32):
 clouds[i]=[(i%4-1.5)*.35, .7+(i//4%4)*.25, (i//16-.5)*.6, 3., .62,.47,.28,.035]
comparisons=0
for yaw in [0.,.4,1.2,2.7]:
 for n in [0,1,3,16,32]:
  for depth in [0.,.99,1.]:
   for transmittance in [False,True]:
    kwargs=dict(clouds=clouds[:n],depth=depth,transmittance=transmittance,pitch=.7)
    prog,vao=optimized_prog,optimized_vao;optimized=render(yaw,**kwargs)
    prog,vao=reference_prog,reference_vao;old=render(yaw,**kwargs)
    assert np.array_equal(optimized,old), 'shared detail noise must preserve every output byte'
    comparisons+=1
prog,vao=optimized_prog,optimized_vao
if args.benchmark:
 # Interleave variants, discard two warmup rounds, no flaky speed threshold.
 # GPU timer covers draw only, not shader build/readback/fixture creation.
 for label,fixture in [('dense32',clouds),('sparse3',clouds[:3].copy())]:
  if label=='sparse3':fixture[:,3]=.45
  timings={'original':[],'shared':[]}
  for iteration in range(12):
   for name,p,v in [('original',reference_prog,reference_vao),('shared',optimized_prog,optimized_vao)]:
    prog,vao=p,v;batch=[]
    for _ in range(4):render(.4,clouds=fixture,depth=1.,pitch=.7,gpu_times=batch)
    if iteration>=2:timings[name].extend(batch)
  for name,values in timings.items():
   print(label,name,'draw ms median',np.median(values),'p10/p90',np.percentile(values,[10,90]),'N',len(values))
 prog,vao=optimized_prog,optimized_vao
reference_vao.release();reference_prog.release()
print('PASS shared-detail equivalence:',comparisons,'dense/empty/occluded GPU outputs')
assert np.array_equal(render(0,count=0),pixels)
assert np.array_equal(render(0,density=0),pixels)
assert np.array_equal(render(0,depth=0),pixels),'opaque foreground must occlude cloud'
a=render(0);b=render(np.pi/2);
assert not np.array_equal(a,render(0,offset=3.)), 'camera translation must change world-fixed cloud projection'
assert np.count_nonzero(render(0,large=True)[:,:,:3]!=pixels[:,:,:3])>np.count_nonzero(a[:,:,:3]!=pixels[:,:,:3])
assert np.any(a!=pixels) and np.any(b!=pixels)
assert np.array_equal(a,render(0)),'same world/camera must render deterministically'
assert np.array_equal(a[:,:,3],pixels[:,:,3])
# Camera types share one reconstruction contract. Pitched close chase and
# far trackside views must stay aligned with projected WORLD cloud centres.
for yaw,pitch in [(0,0),(.7,3),(-.8,-.5)]:
 trans=render(yaw,pitch=pitch,transmittance=True)
 eye=np.array([np.sin(yaw)*7,2.+pitch,np.cos(yaw)*7]);target=np.array([0.,1.,0.]);f=target-eye;f/=np.linalg.norm(f);r=np.cross(f,[0,1,0]);r/=np.linalg.norm(r);u=np.cross(r,f)
 centre=np.array([0.,1.3,.3]);v=centre-eye;z=np.dot(f,v)
 px=int((np.dot(r,v)*1.4*h/w/z+1)*w/2);py=int((np.dot(u,v)*1.4/z+1)*h/2)
 assert 0<=px<w and 0<=py<h and trans[py,px]<250, 'top-origin ray must meet projected cloud centre'
# Removing only the clip-Y correction must break projection/depth alignment.
correct=render(.4,pitch=3.,transmittance=True)
working=prog;working_vao=vao
broken=ctx.program(vertex_shader=vert,fragment_shader=frag.replace('1.0 - (uv.y * 2.0)', '(uv.y * 2.0) - 1.0'))
# SPIRV-Cross may choose a vector constructor spelling. Require actual mutation.
assert '1.0 - (uv.y * 2.0)' in frag, 'update the intentional-regression replacement for emitted GLSL'
broken['DustVolumeUniforms'].binding=0;prog=broken;vao=ctx.vertex_array(broken,[])
wrong=render(.4,pitch=3.,transmittance=True)
if args.output:
 before=render(.4,pitch=3.)
prog=working;vao=working_vao
if args.output:
 after=render(.4,pitch=3.);contact=Image.new('RGB',(960,320))
 for i,(array,label) in enumerate([(before,'BEFORE: clip-Y mismatch | pitched camera'),(after,'AFTER: top-origin SDL GPU ray | pitched camera')]):
  im=Image.fromarray(array).transpose(Image.Transpose.FLIP_TOP_BOTTOM).convert('RGB').resize((480,320));ImageDraw.Draw(im).text((8,8),label,fill='white');contact.paste(im,(480*i,0))
 contact.save(str(Path(args.output).with_name('dust-top-origin-before-after.png')))
assert np.count_nonzero(correct!=wrong)>w*h*.02, 'old Y mapping must visibly fail with a pitched camera'
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
