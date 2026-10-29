#!/usr/bin/env python3
"""Reproducible, exact-source-gated Ballon paired-road preparation.
No artwork bundled. FIN unchanged, experimental handling/native acceptance open.
"""
from pathlib import Path
import struct,json,hashlib,collections,gzip,tempfile,sys
if sys.version_info<(3,10):raise SystemExit(78)
try:
    import numpy as np
except ImportError:
    raise SystemExit('Curved roads need an existing Python3.10+ environment with numpy; nothing was installed.')
from road_contract import SOURCE_HASHES
from road_pair import validate_pair
OUTPUT_HASHES={"curved.hpc":"5b051ddd0cc7749e1ef70f7e8da902ade7fbe039ee82cf8edc877ad09f6d5a18","curved.c3d":"b694283293c0f9bbfbfcd1dbeaf3fba974dd23d632fc5d2d7dbf7e39a0c617a7"}

def write_render(b,cv,cf,newv,newf,source,pairs,new_edges):
    nodes,nm,no,ns=struct.unpack_from('<4H',b,24);nvflag,nv=struct.unpack_from('<II',b,16);nt=struct.unpack_from('<I',b,32)[0];nr=struct.unpack_from('<H',b,38)[0];np_=struct.unpack_from('<H',b,40)[0]
    assert nodes==no==nvflag==nr==np_==0 and nm==174 and ns==800
    ms=48;ss=ms+nm*288;ts=ss+ns*136;vs=ts+nt*76;ls=vs+nv*48;end=ls+nv*4
    assert end==struct.unpack_from('<I',b,8)[0]
    by_render={int(ri):int(ci)for ri,ci in pairs};children=collections.defaultdict(list)
    for face,ci in zip(newf,source):children[int(ci)].append(face)
    headers=bytearray(b[ms:ss]);triangles=bytearray();vertices=bytearray();lights=bytearray();global_ri=0;mid_count=0;max_mesh=0
    for mi in range(nm):
     m=ms+mi*288;vo,vc=struct.unpack_from('<II',b,m+12);to,tc=struct.unpack_from('<II',b,m+36);lo=struct.unpack_from('<I',b,m+52)[0]
     assert vo%48==to%76==lo%4==0 and to//76==global_ri
     vv=bytearray(b[vs+vo:vs+vo+vc*48]);ll=bytearray(b[ls+lo:ls+lo+vc*4]);tt=bytearray();local_mid={};ntri=0
     for ti in range(tc):
      ri=global_ri+ti;tr=b[ts+to+ti*76:ts+to+(ti+1)*76];ids=struct.unpack_from('<3H',tr,64)
      if ri not in by_render:
       tt+=tr;ntri+=1;continue
      ci=by_render[ri];original_ids=cf[ci];mapping={}
      for vi in ids:
       xyz=struct.unpack_from('<3f',vv,vi*48);q=np.rint(np.array(xyz)*65536).astype(np.int64)
       hits=[int(i)for i in original_ids if np.array_equal(q,np.rint(cv[i]*65536).astype(np.int64))];assert len(hits)==1;mapping[hits[0]]=int(vi)
      assert len(mapping)==3
      for face in children[ci]:
       outids=[];weights=[]
       for vi in face:
        vi=int(vi)
        if vi<len(cv):outids.append(mapping[vi]);weights.append({mapping[vi]:1.});continue
        ea,eb=new_edges[vi];assert ea in mapping and eb in mapping
        a,z=mapping[ea],mapping[eb];edge=tuple(sorted((a,z)));weights.append({a:.5,z:.5})
        if edge not in local_mid:
         ai=a*48;zi=z*48;v=bytearray(vv[ai:ai+48]);pa=np.array(struct.unpack_from('<6f',vv,ai));pz=np.array(struct.unpack_from('<6f',vv,zi));p=(pa+pz)/2;p[:3]=newv[vi];normal=p[3:6];length=np.linalg.norm(normal)
         if length>1e-12:p[3:6]/=length
         struct.pack_into('<6f',v,0,*p)
         for off in [24,28]:
          for k in range(4):v[off+k]=(int(vv[ai+off+k])+int(vv[zi+off+k])+1)//2
         for off in [32,36,40,44]:struct.pack_into('<f',v,off,(struct.unpack_from('<f',vv,ai+off)[0]+struct.unpack_from('<f',vv,zi+off)[0])/2)
         local_mid[edge]=len(vv)//48;vv+=v;lv=(struct.unpack_from('<i',ll,a*4)[0]+struct.unpack_from('<i',ll,z*4)[0])//2;ll+=struct.pack('<i',lv);mid_count+=1
        outids.append(local_mid[edge])
       out=bytearray(tr);struct.pack_into('<3H',out,64,*outids)
       for k,w in enumerate(weights):
        for channel in range(4):out[52+k*4+channel]=int(sum(tr[52+ids.index(a)*4+channel]*weight for a,weight in w.items())+.5)
       tt+=out;ntri+=1
     global_ri+=tc;assert len(vv)//48<2000 # actual Mesh_UploadVertices contract
     hm=mi*288;struct.pack_into('<II',headers,hm+12,len(vertices),len(vv)//48);struct.pack_into('<II',headers,hm+36,len(triangles),ntri);struct.pack_into('<I',headers,hm+52,len(lights));struct.pack_into('<i',headers,hm+256,0)
     # Original byte-for-byte vertex/light prefix retained in every mesh.
     assert vv[:vc*48]==b[vs+vo:vs+vo+vc*48] and ll[:vc*4]==b[ls+lo:ls+lo+vc*4]
     vertices+=vv;lights+=ll;triangles+=tt;max_mesh=max(max_mesh,len(vv)//48)
    assert global_ri==nt
    header=bytearray(b[:48]);shift=len(triangles)+len(vertices)+len(lights)-(nt*76+nv*52)
    for offset in [8,12]:struct.pack_into('<I',header,offset,struct.unpack_from('<I',b,offset)[0]+shift)
    struct.pack_into('<I',header,20,len(vertices)//48);struct.pack_into('<I',header,32,len(triangles)//76)
    out=header+headers+b[ss:ts]+triangles+vertices+lights+b[end:]
    assert out[struct.unpack_from('<I',out,8)[0]:]==b[end:]
    return bytes(out)

def build(sources):
    b=sources['temp.c3d'];h=sources['fra01.hpc'];counts=struct.unpack_from('<5h',h,8);p=20+sum(counts)*8
    nv=struct.unpack_from('<H',h,p)[0];p+=4;cv=np.frombuffer(h,dtype='<i4',count=nv*3,offset=p).reshape(-1,3).astype('float64')/65536;p+=nv*12
    nt=struct.unpack_from('<H',h,p)[0];p+=4;words=np.frombuffer(h,dtype='<u2',count=nt*4,offset=p).reshape(-1,4);cf=words[:,:3];xyz={tuple(np.rint(v*65536).astype('int64')):i for i,v in enumerate(cv)}
    # Exact source triplets, no nearest-neighbour layer guessing.
    collision={tuple(sorted(map(int,t))):i for i,t in enumerate(cf)};nm,no,ns=struct.unpack_from('<3H',b,26);ts=48+nm*288+no*448+ns*136;vs=ts+struct.unpack_from('<I',b,32)[0]*76
    verts=[];faces=[];weld={};road=[];pairs=[];render_i=0
    for mi in range(nm):
        m=48+mi*288;vo,vc=struct.unpack_from('<II',b,m+12);to,tc=struct.unpack_from('<II',b,m+36)
        for ti in range(tc):
            off=ts+to+ti*76;ids=struct.unpack_from('<3H',b,off+64);face=[];contact=[]
            for ix in ids:
                point=struct.unpack_from('<3f',b,vs+vo+ix*48);key=tuple(round(x,4)for x in point)
                if key not in weld:weld[key]=len(verts);verts.append(point)
                face.append(weld[key]);contact.append(xyz.get(tuple(round(x*65536)for x in point),-1))
            faces.append(face);road.append(struct.unpack_from('<i',b,off+4)[0]<12)
            ci=collision.get(tuple(sorted(contact)))
            if ci is not None:pairs.append((render_i,ci))
            render_i+=1
    if len(pairs)!=len(cf) or len({ci for ri,ci in pairs})!=len(cf):raise ValueError('Source render/contact triplets differ')
    v=np.array(verts);edges=collections.defaultdict(list)
    for i,face in enumerate(faces):
        for a,z in zip(face,face[1:]+face[:1]):edges[tuple(sorted((a,z)))].append(i)
    boundary={e for e,ts_ in edges.items()if any(road[i]for i in ts_)and any(not road[i]for i in ts_)};adj=collections.defaultdict(list)
    for a,z in boundary:adj[a].append(z);adj[z].append(a)
    moved={}
    for a,z in sorted(boundary):
        if len(adj[a])!=2 or len(adj[z])!=2:continue
        pa=next(x for x in adj[a]if x!=z);pz=next(x for x in adj[z]if x!=a);offset=(-v[pa]+v[a]+v[z]-v[pz])/16;offset[1]=0;length=np.linalg.norm(offset)
        if length>.35:offset*=.35/length
        if np.linalg.norm(offset)<1e-8:continue
        ca=xyz.get(tuple(np.rint(v[a]*65536).astype('int64')),-1);cz=xyz.get(tuple(np.rint(v[z]*65536).astype('int64')),-1)
        if ca<0 or cz<0:continue
        point=(v[a]+v[z])/2+offset
        # Canonical shared new coordinate representable in both formats.
        for _ in range(8):point=np.rint(point.astype('float32').astype('float64')*65536)/65536
        if not np.array_equal(point,point.astype('float32').astype('float64')):raise ValueError('Canonical point failed')
        moved[tuple(sorted((ca,cz)))]=point
    new_edges={len(cv)+i:e for i,e in enumerate(sorted(moved))};cmid={e:i for i,e in new_edges.items()};newv=np.concatenate([cv,np.array([moved[e]for e in sorted(moved)])]);new=[];source=[]
    for i,face in enumerate(cf):
        a,z,d=map(int,face);az=cmid.get(tuple(sorted((a,z))));zd=cmid.get(tuple(sorted((z,d))));da=cmid.get(tuple(sorted((d,a))));mask=(az is not None)+2*(zd is not None)+4*(da is not None)
        splits={0:[[a,z,d]],1:[[a,az,d],[az,z,d]],2:[[z,zd,a],[zd,d,a]],4:[[d,da,z],[da,a,z]],3:[[az,z,zd],[a,az,zd],[a,zd,d]],5:[[da,a,az],[z,d,da],[z,da,az]],6:[[zd,d,da],[a,z,zd],[a,zd,da]],7:[[a,az,da],[az,z,zd],[da,zd,d],[az,zd,da]]}[mask]
        new.extend(splits);source.extend([i]*len(splits))
    new=np.array(new);source=np.array(source)
    if len(new)>=32768 or len(newv)>=65536:raise ValueError('Road index limit exceeded')
    src=cv[cf];dst=newv[new];area=lambda t:(t[:,1,0]-t[:,0,0])*(t[:,2,2]-t[:,0,2])-(t[:,1,2]-t[:,0,2])*(t[:,2,0]-t[:,0,0])
    a0=area(src)[source];a1=area(dst)
    if (((a0*a1)<=0)&(abs(a0)>1e-8)).any():raise ValueError('Road split inverted a source triangle')
    nodes=[];pos=20
    for n in counts:nodes.append([list(struct.unpack_from('<hhi',h,pos+i*8))for i in range(n)]);pos+=n*8
    cols,rows=struct.unpack_from('<hh',h,len(h)-4);x,z=struct.unpack_from('<ii',h);refs=[];mins=dst[:,:,[0,2]].min(1);maxs=dst[:,:,[0,2]].max(1)
    def visit(level,i,lo,side):
        node=nodes[level][i]
        if node[0]==-1:
            for zz in range(4):
                for xx in range(4):visit(level+1,node[2]+zz*4+xx,lo+np.array([xx,zz])*side/4,side/4)
        else:
            ids=np.where(((maxs>=lo-1e-5)&(mins<=lo+side+1e-5)).all(1))[0];node[0]=len(ids);node[2]=len(refs);refs.extend(ids.tolist())
    for zz in range(rows):
        for xx in range(cols):visit(0,zz*cols+xx,np.array([x,z])/65536+np.array([xx,zz])*256,256)
    out=bytearray(h[:20])
    for level in nodes:
        for node in level:out+=struct.pack('<hhi',*node)
    out+=struct.pack('<HH',len(newv),0)+np.rint(newv*65536).astype('<i4').tobytes()+struct.pack('<HH',len(new),0)
    for face,ci in zip(new,source):out+=struct.pack('<4H',*face,int(words[ci,3]))
    out+=struct.pack('<i',len(refs))+np.array(refs,dtype='<i2').tobytes()+struct.pack('<hh',cols,rows)
    render=write_render(b,cv,cf,newv,new,source,pairs,new_edges);validate_pair(out,render)
    return bytes(out),render

def extract(path):
    with gzip.open(path,'rb')as stream:raw=stream.read(64*1024*1024+1)
    if len(raw)>64*1024*1024 or raw[:4]!=b'CMPR':raise ValueError('Unsupported road archive')
    b=raw[8:];directory=struct.unpack_from('<I',b,len(b)-4)[0];pos=directory;found={};seen=set()
    while pos<len(b)-4:
        size,offset,n=struct.unpack_from('<III',b,pos);pos+=12
        if n<1 or n>255 or pos+(n+3)//4*4>len(b)-4:raise ValueError('Invalid road archive directory')
        name=b[pos:pos+n].decode('ascii').lower();pos+=(n+3)//4*4
        if name in seen or offset+size>directory:raise ValueError('Duplicate or invalid road member')
        seen.add(name)
        if name in SOURCE_HASHES:
            member=b[offset:offset+size]
            if hashlib.sha256(member).hexdigest()!=SOURCE_HASHES[name]:raise ValueError('Unsupported road source '+name)
            found[name]=member
    if set(found)!=set(SOURCE_HASHES):raise ValueError('Missing supported Ballon source members')
    return found

def prepare(data,cache):
    data=Path(data).resolve();cache=Path(cache)
    if cache.is_symlink()or any(p.is_symlink()for p in cache.parents):raise ValueError('Symlink road cache refused')
    cache=cache.resolve()
    if cache==data or data in cache.parents:raise ValueError('Road cache must be outside owned data')
    candidates=[p for p in data.rglob('*')if p.name.lower()in ('fra01hi.bfl','fra01hi (1).bfl')]
    if len(candidates)!=1:raise ValueError('Need exactly one owned fra01hi.bfl Ballon archive')
    if candidates[0].is_symlink():raise ValueError('Symlink road archive refused')
    sources=extract(candidates[0]);key=hashlib.sha256()
    for n in ['road_prepare.py','road_pair.py','road_contract.py']:key.update(Path(__file__).with_name(n).read_bytes())
    for n,h in sorted(SOURCE_HASHES.items()):key.update((n+h).encode())
    folder=cache/('roads-curved-'+key.hexdigest()[:24]);cache.mkdir(parents=True,exist_ok=True)
    if folder.is_symlink():raise ValueError('Symlink road pack refused')
    if folder.exists():
        marker=json.loads((folder/'prepared.json').read_text())
        for n in ['curved.hpc','curved.c3d']:
            p=folder/n
            if marker!=OUTPUT_HASHES or p.is_symlink()or hashlib.sha256(p.read_bytes()).hexdigest()!=OUTPUT_HASHES[n]:raise ValueError('Road cache changed; refusing reuse')
        validate_pair((folder/'curved.hpc').read_bytes(),(folder/'curved.c3d').read_bytes())
    else:
        contact,render=build(sources)
        with tempfile.TemporaryDirectory(prefix='.roads-',dir=cache)as tmp:
            stage=Path(tmp)/'pack';stage.mkdir();(stage/'curved.hpc').write_bytes(contact);(stage/'curved.c3d').write_bytes(render)
            marker={n:hashlib.sha256((stage/n).read_bytes()).hexdigest()for n in ['curved.hpc','curved.c3d']}
            if marker!=OUTPUT_HASHES:raise ValueError('Recipe output differs from reviewed pair; refusing activation')
            (stage/'prepared.json').write_text(json.dumps(marker,sort_keys=True))
            # Runtime consumes a strict bounded line protocol, not JSON guesses.
            (stage/'road-pair.tsv').write_text('OPENCMR2-ROAD-PAIR-1\n'+marker['curved.hpc']+'\t'+marker['curved.c3d']+'\n')
            if folder.exists():raise ValueError('Road cache created concurrently; retry')
            stage.rename(folder)
    print('Experimental Ballon curved roads: paired contact/render, FIN unchanged; native driving/performance unverified')
    print('OPENCMR2_ROADS='+str(folder));return folder

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--input-dir',type=Path,required=True);p.add_argument('--cache-dir',type=Path,required=True);a=p.parse_args()
    try:prepare(a.input_dir,a.cache_dir)
    except (ValueError,OSError,AssertionError,struct.error)as e:raise SystemExit('Road preparation refused: '+str(e))
