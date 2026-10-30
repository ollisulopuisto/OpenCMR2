"""Portable integer-only source2x filter, no native floating-point resize.
RGBA input, premultiplied bilinear top level, area mips. Alpha coverage is
preserved per mip using exact rational comparisons. Original samples, dimensions and UV layout stay put.
All round-to-nearest ties round up. Explicit clamp/repeat addressing is supported.
"""
from PIL import Image
import numpy as np
from fractions import Fraction
from collections import Counter

def half(a):
 h,w,_=a.shape
 # A one-pixel axis is repeated, preserving constant colour/alpha.
 x=np.minimum(np.arange(max(1,w//2))*2,w-1);y=np.minimum(np.arange(max(1,h//2))*2,h-1)
 return (a[y[:,None],x]+a[y[:,None],np.minimum(x+1,w-1)]+a[np.minimum(y+1,h-1)[:,None],x]+a[np.minimum(y+1,h-1)[:,None],np.minimum(x+1,w-1)]+2)//4

def levels(image,repeat=(False,False)):
 src=np.array(image.convert('RGBA'),dtype=np.int64);h,w,_=src.shape
 # Premultiply without division so alpha association loses no source bits.
 p=src.copy();p[:,:,:3]*=p[:,:,3:4]
 def axis(a,axis):
  n=a.shape[axis];i=np.arange(n*2);lo=(i-1)//2;hi=(i+1)//2
  if repeat[0 if axis==1 else 1]:lo=lo%n;hi=hi%n
  else:lo=np.maximum(0,lo);hi=np.minimum(n-1,hi)
  # Pixel-centre mapping: odd output samples lie 1/4 after source centre,
  # even ones 1/4 before it. Endpoints clamp to the same source sample.
  weight=np.where(i%2,1,3);shape=[1]*a.ndim;shape[axis]=n*2;weight=weight.reshape(shape)
  return (np.take(a,lo,axis=axis)*(4-weight)+np.take(a,hi,axis=axis)*weight+2)//4
 target=int(np.count_nonzero(src[:,:,3]>=128));pixels=w*h
 a=axis(axis(p,1),0);out=[]
 while True:
  rgba=np.empty(a.shape,dtype=np.uint8);alpha=a[:,:,3];rgba[:,:,3]=np.clip(alpha,0,255)
  den=np.maximum(1,alpha[:,:,None]);rgb=(a[:,:,:3]+den//2)//den;rgb=np.where(alpha[:,:,None]>0,rgb,0);rgba[:,:,:3]=np.clip(rgb,0,255)
  if out and target!=pixels:
   hist=Counter(int(v)for v in alpha.flat);wanted=(target*alpha.size*2+pixels)//(pixels*2)
   scales={Fraction(1)}|{Fraction(255,2*v)for v in hist if v}
   def rounded(v,s):return min(255,(v*s.numerator*2+s.denominator)//(2*s.denominator))
   scale=min(scales,key=lambda s:(abs(sum(c for v,c in hist.items()if rounded(v,s)>=128)-wanted),abs(s-1),s))
   lut=np.array([rounded(v,scale)for v in range(256)],dtype=np.uint8);rgba[:,:,3]=lut[alpha]
  out.append(Image.fromarray(rgba))
  if a.shape[:2]==(1,1):return out
  a=half(a)
