#!/usr/bin/env python3
"""Count strong candidate stereo clicks in WAV. Requires numpy + soundfile.
Usage: python3 tools/analyze_wav.py 'sample 017.wav' 'sample 019.wav'
Not a perceptual quality metric; see docs/testing-method.md.
"""
import sys
import numpy as np
import soundfile as sf

def count_clicks(x):
    if x.ndim!=2 or x.shape[1]<2:raise ValueError('requires stereo audio')
    z=x[:,:2]
    cur=z[1:-1];left=z[:-2];right=z[2:]
    error=np.abs(cur-(left+right)*0.5)
    sharp=np.abs(cur-left)+np.abs(cur-right)
    cand=np.where(np.all((error>0.20)&(sharp>0.40),axis=1))[0]+1
    n=0;last=-100
    for frame in cand:
        if frame-last>2:n+=1
        last=frame
    return n
for f in sys.argv[1:]:
    x,sr=sf.read(f,dtype='float64',always_2d=True)
    print(f, 'duration_s=',round(len(x)/sr,3), 'candidates=',count_clicks(x))
