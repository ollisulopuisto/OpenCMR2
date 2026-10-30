"""Full-canvas cutout2x filter. No atlas/crop/repeat inference and no new detail.
Premultiplied RGB prevents transparent texels bleeding into foliage edges.
Mips preserve the nearest attainable >=128 alpha coverage, never shift UVs.
"""
from PIL import Image
from collections import Counter

def resize_cutout(image,size):
 return image.convert('RGBA').convert('RGBa').resize(size,Image.Resampling.LANCZOS).convert('RGBA')

def levels(image):
 image=image.convert('RGBA');target=sum(a>=128 for a in image.getchannel('A').getdata())/(image.width*image.height)
 top=resize_cutout(image,(image.width*2,image.height*2));out=[];w,h=top.size
 while True:
  im=resize_cutout(top,(w,h))
  if out:
   values=list(im.getchannel('A').getdata());wanted=round(target*len(values));histogram=Counter(values);candidates={1.0}
   # Discrete thresholds: choose closest coverage, then smallest alpha change.
   for a in histogram:
    if a:candidates.add(127.5/a)
   scale=min(candidates,key=lambda s:(abs(sum(count for a,count in histogram.items() if min(255,int(a*s+.5))>=128)-wanted),abs(s-1)))
   im.putalpha(Image.frombytes('L',(w,h),bytes(min(255,int(a*scale+.5))for a in values)))
  out.append(im)
  if(w,h)==(1,1):return out
  w=max(1,w//2);h=max(1,h//2)
