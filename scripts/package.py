"""Package an installed tree; Windows additionally bundles toolchain DLLs."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--build-dir', default='build')
parser.add_argument('--name', required=True)
parser.add_argument('--toolchain', type=Path)
args = parser.parse_args()
project = Path(__file__).resolve().parents[1]
stage = project / 'dist' / args.name
if stage.exists():
    raise SystemExit(f'Refusing to mix package contents with existing {stage}')
stage.mkdir(parents=True)
subprocess.run(['cmake', '--install', str(args.build_dir), '--prefix', str(stage)], check=True)
licenses = stage / 'licenses'
licenses.mkdir()
for source in ('vendor/ejs', 'vendor/pugixml'):
    for path in (project / source).glob('*'):
        if path.name.startswith(('LICENSE', 'PROVENANCE')):
            shutil.copy2(path, licenses / (path.parent.name + '-' + path.name))
if os.name == 'nt':
    if not args.toolchain:
        raise SystemExit('--toolchain is required for Windows DLL packaging')
    toolbin = args.toolchain / 'bin'
    queue = [stage / 'bin/ytcui-we.exe']
    seen = set()
    while queue:
        output = subprocess.check_output([str(toolbin / 'objdump.exe'), '-p', str(queue.pop())], text=True)
        for name in re.findall(r'DLL Name:\s*(\S+)', output):
            if name.lower() in seen:
                continue
            seen.add(name.lower())
            source = toolbin / name
            if source.is_file():
                target = stage / 'bin' / name
                shutil.copy2(source, target)
                queue.append(target)
    notices = args.toolchain / 'share/licenses'
    if not notices.is_dir():
        raise SystemExit('MSYS2 dependency license directory missing')
    shutil.copytree(notices, licenses / 'msys2')
subprocess.run([os.sys.executable, str(project / 'test/test_cli.py'),
                str(stage / 'bin' / ('ytcui-we.exe' if os.name == 'nt' else 'ytcui-we'))], check=True)
archive = Path(shutil.make_archive(str(stage), 'zip' if os.name == 'nt' else 'gztar',
                                   root_dir=stage.parent, base_dir=stage.name))
checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_name(archive.name + '.sha256').write_text(f'{checksum}  {archive.name}\n')
# Test the actual archive, rather than only its staging tree.
unpacked = project / 'dist' / ('verify-' + args.name)
shutil.unpack_archive(archive, unpacked)
executable = unpacked / args.name / 'bin' / ('ytcui-we.exe' if os.name == 'nt' else 'ytcui-we')
subprocess.run([os.sys.executable, str(project / 'test/test_cli.py'), str(executable)], check=True)
assets = unpacked / args.name / 'share/ytcui-we/ejs'
assert (assets / 'yt.solver.lib.js').is_file()
assert (assets / 'yt.solver.core.js').is_file()
print(f'Packaged and verified {archive.name}: {checksum}')
