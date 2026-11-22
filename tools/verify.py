#!/usr/bin/env python3
from pathlib import Path
import hashlib,json,struct,subprocess,sys
ROOT=Path(__file__).resolve().parent.parent;A=ROOT/'autorun'
check=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
a=A/'autorun.sh'; s=a.read_text(); runner=A/'pcmfix-run.sh'; lib=A/'libmpc_pcmfix.so'
assert (A/'autorun.check').read_text()==hashlib.sha256((A/'autorun.json').read_bytes()+a.read_bytes()+b'f7bed1c9-7e90-4731-9194-c996666b4331').hexdigest()
assert f"expected='{check(lib)}'" in s
assert f"expected_runner='{check(runner)}'" in s
assert json.loads((A/'autorun.json').read_text())['value0']['scripts'][0]['path']=='autorun.sh'
assert (A/'pcmfix-splash.raw').stat().st_size==640*240*2
elf=lib.read_bytes()[:64]
assert elf[:4]==b'\x7fELF' and elf[4]==2 and struct.unpack_from('<H',elf,18)[0]==183
for p in [a,runner]:
 p=subprocess.run(['sh','-n',str(p)],capture_output=True,text=True)
 assert p.returncode==0, p.stderr
print('verified manifest, pins, scripts, 640x240 RGB565, aarch64 ELF')
