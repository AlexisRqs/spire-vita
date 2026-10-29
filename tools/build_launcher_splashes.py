"""Package the native startup logos for a short post-Play presentation."""
from pathlib import Path
import argparse
from PIL import Image, ImageOps
from build_livearea import decode
p=argparse.ArgumentParser();p.add_argument('assets',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
pixels=[]
for name in ('megaCrit.png','humble_old.png'):
 im=decode(a.assets/'images'/name)
 im=ImageOps.contain(im,(600,320),Image.Resampling.LANCZOS)
 bg=Image.new('RGBA',(640,360),(0,0,0,255));bg.alpha_composite(im,((640-im.width)//2,(360-im.height)//2))
 pixels.append(bg.tobytes())
a.output.parent.mkdir(parents=True,exist_ok=True)
a.output.write_bytes(b'SPLASH01'+b''.join(pixels))
