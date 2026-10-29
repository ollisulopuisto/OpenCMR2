"""Unused bounded paired-road validator/transaction research. No runtime caller."""
import math
import struct
from collections import Counter
from dataclasses import dataclass

MAX_BYTES = 64*1024*1024

def _read(raw, offset, fmt):
    if offset < 0 or offset+struct.calcsize(fmt)>len(raw):
        raise ValueError('Pair table outside buffer')
    return struct.unpack_from(fmt, raw, offset)

def _key(points):
    if any(not math.isfinite(x) or abs(x)>32767 for p in points for x in p):
        raise ValueError('Nonfinite or out-of-range pair coordinate')
    q=tuple(tuple(round(x*65536)for x in p)for p in points)
    return min(q,q[1:]+q[:1],q[2:]+q[:2])

def collision_keys(raw):
    if not raw or len(raw)>MAX_BYTES:
        raise ValueError('Bounded collision buffer required')
    counts=_read(raw,8,'<5h')
    if min(counts)<0:
        raise ValueError('Invalid quadtree count')
    p=20+sum(counts)*8;n=_read(raw,p,'<H')[0];p+=4
    if not 0<n<65536:raise ValueError('Invalid vertex count')
    verts=[tuple(x/65536 for x in _read(raw,p+i*12,'<3i'))for i in range(n)]
    p+=n*12;t=_read(raw,p,'<H')[0];p+=4
    if not 0<t<32768:raise ValueError('Invalid triangle count')
    result=Counter()
    for i in range(t):
        ids=_read(raw,p+i*8,'<4H')[:3]
        if max(ids)>=n:raise ValueError('Invalid collision vertex index')
        result[_key([verts[j]for j in ids])]+=1
    p+=t*8
    refs=_read(raw,p,'<i')[0];p+=4
    if refs<0 or p+refs*2+4!=len(raw):
        raise ValueError('Invalid collision reference inventory')
    for i in range(refs):
        if not 0<=_read(raw,p+i*2,'<h')[0]<t:
            raise ValueError('Collision triangle reference outside table')
    offset=20
    for level,count in enumerate(counts):
        for i in range(count):
            n,unused,start=_read(raw,offset+i*8,'<hhi')
            if n==-1:
                if level==4 or start<0 or start+16>counts[level+1]:
                    raise ValueError('Collision child block outside table')
            elif n<0 or start<0 or start+n>refs:
                raise ValueError('Collision leaf references outside table')
        offset+=count*8
    cols,rows=_read(raw,p+refs*2,'<hh')
    if cols<=0 or rows<=0 or cols*rows!=counts[0]:
        raise ValueError('Invalid collision grid')
    return result

def render_keys(raw):
    if len(raw)>MAX_BYTES or raw[:4]!=b'PP_F':
        raise ValueError('Bounded C3D required')
    nodes,nm,no,ns=_read(raw,24,'<4H');flags,nv=_read(raw,16,'<II');nt=_read(raw,32,'<I')[0];records=_read(raw,38,'<H')[0];portals=_read(raw,40,'<H')[0]
    # Explicitly refuse anything beyond the verified Ballon ground-only layout.
    if nodes or no or flags or records or portals or not 0<nm<=174 or ns>800 or nv>100000 or nt>32767:
        raise ValueError('Unsupported paired render layout')
    ms=48;ss=ms+nm*288;ts=ss+ns*136;vs=ts+nt*76;end=vs+nv*52
    texture_start=_read(raw,8,'<I')[0];name_start=_read(raw,12,'<I')[0]
    if end!=texture_start or not end<=name_start<=len(raw):
        raise ValueError('Render tail offset differs')
    result=Counter();triangle_total=0;vertex_total=0
    for i in range(nm):
        m=ms+i*288;vo,vc=_read(raw,m+12,'<II');to,tc=_read(raw,m+36,'<II');lo=_read(raw,m+52,'<I')[0]
        if vc>=2000 or vo%48 or to%76 or lo%4 or vo+vc*48>nv*48 or to+tc*76>nt*76 or lo+vc*4>nv*4:
            raise ValueError('Render mesh tables outside buffer')
        if vo!=vertex_total*48 or to!=triangle_total*76 or lo!=vertex_total*4:
            raise ValueError('Overlapping or reordered mesh tables')
        vertex_total+=vc;triangle_total+=tc
        for j in range(tc):
            ids=_read(raw,ts+to+j*76+64,'<3H')
            if max(ids)>=vc:raise ValueError('Render triangle index outside mesh')
            result[_key([_read(raw,vs+vo+k*48,'<3f')for k in ids])]+=1
    if triangle_total!=nt or vertex_total!=nv:
        raise ValueError('Incomplete render mesh inventory')
    return result

def validate_pair(hpc,c3d,visual_only=82):
    collision=collision_keys(hpc);render=render_keys(c3d)
    if collision-render or sum((render-collision).values())!=visual_only:
        raise ValueError('Render/contact pair differs')
    return sum(collision.values()),sum(render.values())

@dataclass(frozen=True)
class Selection:
    contact: bytes
    render: bytes
    research: bool
    reason: str

def select(original_contact,original_render,candidate_contact=None,candidate_render=None,*,identity_verified=False,visual_only=82):
    """Pure all-or-original selection, unused by binary. Never edits source bytes.

    identity_verified is the caller's prior exact-source check, NOT authority
    supplied by a sidecar. No producer, loader or global state is invoked here.
    """
    original=Selection(original_contact,original_render,False,'original')
    if not identity_verified:return original
    if candidate_contact is None or candidate_render is None:
        return Selection(original_contact,original_render,False,'incomplete pair')
    # Freeze candidates before validation so returned bytes are those checked.
    candidate_contact=bytes(candidate_contact);candidate_render=bytes(candidate_render)
    try:validate_pair(candidate_contact,candidate_render,visual_only)
    except (ValueError,struct.error,OverflowError):
        return Selection(original_contact,original_render,False,'invalid pair')
    return Selection(bytes(candidate_contact),bytes(candidate_render),True,'validated research pair, runtime unavailable')
