"""Opaque exact-source road2x/mip filter for the optional France1 test.

Caller must establish repeat axes from material/sampler routing. Filenames and
edge error do not establish tileability. Never feed a cutout/atlas through this.
"""
from PIL import Image

def resize(image,size,repeat):
 if repeat not in ((True,False),(False,True),(True,True)):
  raise ValueError('Explicit nonempty repeat-axis policy required')
 rgba=image.convert('RGBA')
 if rgba.getchannel('A').getextrema()!=(255,255):
  raise ValueError('Only opaque single-material source supported')
 if len(size)!=2 or any(type(n)is not int or not 1<=n<=8192 for n in size):
  raise ValueError('Unsupported output dimensions')
 # Lanczos has support radius3 target samples when downsampling. Seven tiles
 # keep the center tile interior even at a1x1 mip. Nonrepeat axes retain the
 # same edge clamp as Pillow. Repeat wrapping is filtering, not forced edge RGB.
 nx=7 if repeat[0] else 1;ny=7 if repeat[1] else 1
 tiled=Image.new('RGB',(rgba.width*nx,rgba.height*ny));rgb=rgba.convert('RGB')
 for y in range(ny):
  for x in range(nx):tiled.paste(rgb,(x*rgb.width,y*rgb.height))
 scaled=tiled.resize((size[0]*nx,size[1]*ny),Image.Resampling.LANCZOS)
 x=(nx//2)*size[0];y=(ny//2)*size[1]
 return scaled.crop((x,y,x+size[0],y+size[1])).convert('RGBA')

def mip_chain(image,repeat):
 levels=[];w,h=image.size
 while True:
  levels.append(resize(image,(w,h),repeat))
  if(w,h)==(1,1):return levels
  w=max(1,w//2);h=max(1,h//2)
