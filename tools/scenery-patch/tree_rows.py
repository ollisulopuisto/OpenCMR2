"""Private Finland forest-row pack: hand reconstruction, never automatic segmentation."""
from pathlib import Path
import argparse, io, json, math, tempfile, shutil
import numpy as np
from PIL import Image
import convert as cv

ROWS={'tresh1ba':('tre_fir1','tre_fir2','tre_fr7'), 'tresh2fa':('tre_wb1','tre_wb4','tre_wb6')}
PLACEMENTS=((-3.3,.25,6.2,0),(-1.65,-.4,7.3,35),(-.1,.35,6.6,80),(1.7,-.2,7.7,125),(3.3,.5,6.4,170))

def compose(meshes):
    """Return source-canvas-normalized vertices, preserving each mesh's materials/UVs."""
    result=[]
    for i,(x,z,h,yaw) in enumerate(PLACEMENTS):
        co,si=math.cos(math.radians(yaw)),math.sin(math.radians(yaw))
        for mat,p,uv,n in meshes[i%len(meshes)]:
            px,py,pz=p;nx,ny,nz=n
            result.append((str(i)+'_'+mat,((px*co+pz*si)*h+x,py*h,(-px*si+pz*co)*h+z),uv,(nx*co+nz*si,ny,-nx*si+nz*co)))
    points=np.array([r[1] for r in result]);lo=points.min(0);hi=points.max(0)
    # Uniform scaling fits both the source width and height, never stretches crowns.
    scale=min(.96/(hi[0]-lo[0]),.98/(hi[1]-lo[1]))
    center=(hi[0]+lo[0])/2
    result=[(m,((p[0]-center)*scale,(p[1]-lo[1])*scale,p[2]*scale),uv,n) for m,p,uv,n in result]
    if any(abs(p[0])>.5 or not 0<=p[1]<=1 or abs(p[2])>.5 for _,p,_,_ in result):raise ValueError('row exceeds bounded canvas')
    return result

def read_mesh(path):
    p=[];uv=[];n=[];faces=[];mat=''
    for line in path.read_text().splitlines():
        a=line.split()
        if not a:continue
        if a[0]=='v':p.append(tuple(map(float,a[1:])))
        elif a[0]=='vt':uv.append(tuple(map(float,a[1:])))
        elif a[0]=='vn':n.append(tuple(map(float,a[1:])))
        elif a[0]=='usemtl':mat=a[1]
        elif a[0]=='f':
            for token in a[1:]:
                x,y,z=(int(v)-1 for v in token.split('/'));faces.append((mat,p[x],uv[y],n[z]))
    return faces

def generate(data,index,out):
    old=cv.root;cv.root=out
    try:
        manifest=['kind\tsource_archive\tsource_texture\tasset\tframes\tticks'];ids=[];coverage=[]
        with tempfile.TemporaryDirectory() as td:
            meshes={}
            for name in sorted({n for names in ROWS.values() for n in names}):
                key='finhit.bfl/'+name+'.dds';source=Path(td)/(name+'.png')
                Image.open(io.BytesIO(data[key])).convert('RGBA').save(source)
                rel=Path('row-trees')/name
                cv.tree({'path':str(source),'texture':name+'.dds'},rel,adaptive=True,stem=True)
                meshes[name]=read_mesh(out/rel/'tree.obj')
            for strip,names in ROWS.items():
                row=compose([meshes[n] for n in names]);dest=out/'rows'/strip;dest.mkdir(parents=True)
                lines=['# opencmr2-source-canvas-1 256 256','mtllib row.mtl']
                for tag,j in [('v',1),('vt',2),('vn',3)]:lines += [tag+' '+' '.join(f'{v:.8f}' for v in r[j]) for r in row]
                mat=None
                for i,r in enumerate(row):
                    if i%3==0:
                        if mat!=r[0]:mat=r[0];lines.append('usemtl '+mat)
                        lines.append('f '+' '.join(f'{j}/{j}/{j}' for j in range(i+1,i+4)))
                (dest/'row.obj').write_text('\n'.join(lines)+'\n')
                materials=[]
                for i,name in enumerate(names):
                    for mat in ('crown','core','cap','trunk'):
                        leaf=f'{i}_{mat}.tga';shutil.copyfile(out/'row-trees'/name/(mat+'.tga'),dest/leaf)
                        for placement in range(i,5,len(names)):
                            materials += [f'newmtl {placement}_{mat}','Kd 1 1 1','map_Kd '+leaf]
                (dest/'row.mtl').write_text('\n'.join(materials)+'\n')
                # Both known resolutions route to the high-source reconstruction, with exact DDS hashes.
                for archive in ('finhit.bfl','finlot.bfl'):
                    key=archive+'/'+strip+'.dds'
                    manifest.append('\t'.join(['tree-row',archive,strip+'.dds',f'rows/{strip}/row.obj','','1']))
                    ids.append('\t'.join([key,str(len(data[key])),index[key]['sha256']]))
                coverage.append({'strip':strip,'trees':5,'triangles':len(row)//3,'placement':'hand reconstructed, not segmented','sources':names})
        shutil.rmtree(out/'row-trees')
        (out/'manifest.tsv').write_text('\n'.join(manifest)+'\n');(out/'source-identities.tsv').write_text('\n'.join(ids)+'\n')
        (out/'row-coverage.json').write_text(json.dumps(coverage,indent=2)+'\n')
    finally:cv.root=old

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--input-dir',type=Path,required=True);ap.add_argument('--output-dir',type=Path,required=True);args=ap.parse_args()
    inp=args.input_dir.resolve();dest=args.output_dir.resolve()
    if not inp.is_dir() or dest==inp or inp in dest.parents:ap.error('Input must exist; output must be outside originals')
    if dest.exists():ap.error('Output must be new; existing files are never replaced')
    index=json.loads(Path(__file__).with_name('source-index.json').read_text());data={}
    for archive in ('finhit.bfl','finlot.bfl'):
        path=inp/archive
        if not path.is_file() or path.is_symlink():ap.error('Need unlinked '+archive+' directly under input')
        data.update(cv.read_archive(path,archive,index))
    dest.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.tree-row-',dir=dest.parent) as td:
        pack=Path(td)/'pack';pack.mkdir();generate(data,index,pack)
        if dest.exists():raise ValueError('Output appeared during generation')
        pack.rename(dest)
    print('Prepared private hand-reconstructed row pack:',dest)
if __name__=='__main__':main()
