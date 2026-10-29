"""Export the user's native title atlas and logo to Vita packaging dimensions.
No painting/generative changes; original asset layers, deterministic layout.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from PIL import Image,ImageOps
import texture2ddecoder as decoder

def decode(p):
 b=p.read_bytes();assert b[:12]==b'\xabKTX 11\xbb\r\n\x1a\n';fmt=struct.unpack_from('<I',b,28)[0];w,h=struct.unpack_from('<II',b,36);off=68+struct.unpack_from('<I',b,60)[0]
 f={0x9274:decoder.decode_etc2,0x9278:decoder.decode_etc2a8}[fmt]
 return Image.frombytes('RGBA',(w,h),f(b[off:],w,h),'raw','BGRA')
def save(im,p,alpha=False):
 p.parent.mkdir(parents=True,exist_ok=True)
 if alpha: im.quantize(colors=128,method=Image.Quantize.FASTOCTREE,dither=Image.Dither.NONE).save(p,optimize=True)
 else: im.convert('RGB').quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE).save(p,optimize=True)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('assets',type=Path);ap.add_argument('output',type=Path);ap.add_argument('preview',type=Path);a=ap.parse_args();root=a.assets
 sky=decode(root/'title/title.jpg').crop((2,2,1922,1471))
 lower=decode(root/'title/title.png').crop((2,2,1922,1142))
 upper=decode(root/'title/title2.png').crop((2,860,1922,2000))
 # A fixed camera view of the native title layers. Adjacent tower pieces meet
 # at y=960. LiveArea has no game camera animation or menu text.
 scene=sky.copy();scene.alpha_composite(upper,(0,-180));scene.alpha_composite(lower,(0,960))
 scene=scene.crop((0,0,1920,1143))
 bg=ImageOps.fit(scene,(840,500),method=Image.Resampling.LANCZOS)
 logo=decode(root/'images/ui/title_logo/eng.png');logo=logo.crop(logo.getbbox())
 # The system owns the launch button; the gate contains only the game logo.
 gate=Image.new('RGBA',(280,158),(0,0,0,0));mark=ImageOps.contain(logo,(244,142),method=Image.Resampling.LANCZOS);gate.alpha_composite(mark,((280-mark.width)//2,(158-mark.height)//2))
 icon=ImageOps.fit(bg,(128,128),method=Image.Resampling.LANCZOS,centering=(.18,.35)).convert('RGBA');mark=ImageOps.contain(logo,(112,96),method=Image.Resampling.LANCZOS);icon.alpha_composite(mark,((128-mark.width)//2,(128-mark.height)//2))
 out=a.output;contents=out/'livearea/contents'
 save(bg,contents/'bg0.png');save(gate,contents/'startup.png',alpha=True);save(icon,out/'icon0.png')
 preview=bg.copy();preview.alpha_composite(gate,(280,171));a.preview.parent.mkdir(parents=True,exist_ok=True);preview.save(a.preview)
 report={'sources':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [root/'title/title.jpg',root/'title/title.png',root/'title/title2.png',root/'images/ui/title_logo/eng.png']},'outputs':{},'description':'Static reconstruction from native title layers, not an in-game screenshot. Preview excludes the system Start button.'}
 for p in [out/'icon0.png',contents/'bg0.png',contents/'startup.png']:
  im=Image.open(p);assert im.mode=='P';assert p.stat().st_size<(65536 if p.name=='startup.png' else 131072)
  report['outputs'][str(p.relative_to(out))]={'size':list(im.size),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
 (out/'source-manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['outputs']))
if __name__=='__main__':main()
