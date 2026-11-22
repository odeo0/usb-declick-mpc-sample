#!/usr/bin/env python3
from pathlib import Path
import hashlib,re
root=Path(__file__).resolve().parent.parent; a=root/'autorun'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
s=(a/'autorun.sh').read_text()
s=re.sub(r"expected='[^']+'",f"expected='{sha(a/'libmpc_pcmfix.so')}'",s)
s=re.sub(r"expected_runner='[^']+'",f"expected_runner='{sha(a/'pcmfix-run.sh')}'",s)
(a/'autorun.sh').write_text(s)
salt=b'f7bed1c9-7e90-4731-9194-c996666b4331'
digest=hashlib.sha256((a/'autorun.json').read_bytes()+(a/'autorun.sh').read_bytes()+salt).hexdigest()
(a/'autorun.check').write_text(digest)
print('USB DeClick v0.1.0-beta.1 manifest:',digest)
