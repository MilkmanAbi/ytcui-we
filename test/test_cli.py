"""Offline checks of the public executable, including a foreign working directory."""
import os
import subprocess
import sys
import tempfile
from pathlib import Path

exe = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='ytcui smoke space ') as temporary:
    env = dict(os.environ)
    env['PATH'] = str(exe.parent) + os.pathsep + env.get('PATH', '')
    for command in (str(exe), exe.name):
        for args, valid in [(['--version'], True), (['--help'], True),
                            (['--this-option-does-not-exist'], False)]:
            result = subprocess.run([command, *args], cwd=temporary, env=env,
                                    capture_output=True, timeout=20)
            assert (result.returncode == 0) == valid, (args, result.stderr)
            if valid:
                assert b'ytcui-we' in result.stdout, result.stdout
                if args == ['--version']:
                    assert b'0.2.0' in result.stdout, result.stdout
print('CLI version, help, invalid arguments and PATH invocation passed')
