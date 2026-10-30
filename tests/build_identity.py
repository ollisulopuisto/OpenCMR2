import pathlib,subprocess,tempfile,sys
script=pathlib.Path(__file__).resolve().parents[1]/'cmake/BuildIdentity.cmake'
def run(*args,cwd=None):return subprocess.check_output(args,cwd=cwd,stderr=subprocess.DEVNULL,text=True)
with tempfile.TemporaryDirectory() as d:
 root=pathlib.Path(d);repo=root/'repo';repo.mkdir();out=root/'id.h'
 def identity():
  run(sys.argv[1],'-DSOURCE='+str(repo),'-DOUTPUT='+str(out),'-P',str(script));return out.read_text()
 assert 'source-archive' in identity()
 run('git','init','-b','identity-test',cwd=repo);run('git','config','user.email','test@example.invalid',cwd=repo);run('git','config','user.name','Test',cwd=repo)
 (repo/'x').write_text('a');run('git','add','x',cwd=repo);run('git','commit','-m','a',cwd=repo)
 sha=run('git','rev-parse','--short=12','HEAD',cwd=repo).strip();a=identity();assert sha in a and 'identity-test' in a and 'clean-tracked' in a
 stamp=out.stat().st_mtime_ns;identity();assert stamp==out.stat().st_mtime_ns
 (repo/'x').write_text('b');assert 'dirty-tracked' in identity()
 run('git','commit','-am','b',cwd=repo);b=identity();assert b!=a and 'clean-tracked' in b
 run('git','tag','vtest',cwd=repo);assert 'tag vtest' in identity()
 run('git','checkout','--detach',cwd=repo);assert 'branch detached' in identity()
 print('archive, commit/branch/tag, dirty edits, unchanged output, checkout refresh pass')
