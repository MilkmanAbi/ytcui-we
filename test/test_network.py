import hashlib,http.server,threading,subprocess,os,json,time,sys,argparse
from pathlib import Path
project=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--build-dir',type=Path,default=project/('build-win' if os.name=='nt' else 'build'))
options=parser.parse_args()
build=options.build_dir.resolve()
root=build/('network-fixtures-'+str(time.time_ns()))
root.mkdir(parents=True,exist_ok=True)
chunk=1024*1024
data=b''.join(hashlib.sha256(str(i).encode()).digest()*(chunk//32) for i in range(3))+b'end-of-fixture-123'
requests=[]
class Server(http.server.BaseHTTPRequestHandler):
 def log_message(self,*a):pass
 def do_GET(self):
  body=data
  if self.path.startswith('/fragment/'):
   body=(f'fragment-{self.path.rsplit("/",1)[-1]}-'.encode())*19
  header=self.headers.get('Range')
  requests.append((self.path,header))
  if header and self.path not in ['/ignore']:
   start,end=map(int,header.removeprefix('bytes=').split('-'))
   end=min(end,len(body)-1)
   self.send_response(206)
   wrong=1 if self.path=='/wrong' else 0
   self.send_header('Content-Range',f'bytes {start+wrong}-{end}/{len(body)}')
   body=body[start:end+1]
  else:self.send_response(200)
  self.send_header('Content-Length',str(len(body)));self.end_headers()
  try:self.wfile.write(body)
  except (BrokenPipeError,ConnectionResetError):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Server)
threading.Thread(target=server.serve_forever,daemon=True).start()
exe=build/('test_download_native.exe' if os.name=='nt' else 'test_download_native')
env=dict(os.environ)
def run(name,path,extra=()):
 target=root/name
 args=[str(exe),f'http://127.0.0.1:{server.server_port}{path}',str(len(data)),str(target),*extra]
 result=subprocess.run(args,env=env,capture_output=True,timeout=60)
 return result,target
results=[]
for name,path in [('ranges.bin','/range'),('ignored-range.bin','/ignore')]:
 result,target=run(name,path)
 assert result.returncode==0,(name,result.stderr)
 assert target.read_bytes()==data,name
 results.append(name+' byte-identical')
result,target=run('invalid-range.bin','/wrong')
assert result.returncode!=0 and not target.exists(),'invalid range was accepted'
results.append('invalid range rejected without finalizing')
resume=root/'resume.bin'
partial=Path(str(resume)+'.part');partial.write_bytes(data[:chunk]+bytes(len(data)-chunk))
identity=hashlib.sha256(f':probe:{len(data)}'.encode()).hexdigest()
Path(str(partial)+'.ranges').write_text(f'{len(data)} {chunk} {identity}\n0\n')
requests.clear();result,target=run('resume.bin','/range')
assert result.returncode==0 and target.read_bytes()==data,result.stderr
assert all(header!='bytes=0-1048575' for path,header in requests),'completed chunk was fetched again'
results.append('resume reuses committed chunks; output byte-identical')
result,target=run('fragments.bin','/fragment',('fragments',))
expected=b''.join((f'fragment-{i}-'.encode())*19 for i in range(3))
assert result.returncode==0 and target.read_bytes()==expected,result.stderr
results.append('native fragmented download byte-identical')
print(json.dumps({'passed':results},indent=2))
server.shutdown()
