"""Extract only the known compatible ARMv7 engine from a user's local APK."""
import argparse, hashlib, zipfile
from pathlib import Path
ENGINE_SHA256='619e71cd19c9209dff17d943f94a6e14f9f6e899add6f3b33c14f909594f9d18'
ENTRY='lib/armeabi-v7a/libSpire_ANDROID.so'
def extract(apk,output):
    output=Path(output)
    if output.exists():raise ValueError('Output exists')
    with zipfile.ZipFile(apk) as z:
        entries=[i for i in z.infolist() if i.filename==ENTRY]
        if len(entries)!=1 or not 0<entries[0].file_size<=64*1024*1024:
            raise ValueError('Missing, duplicate or oversized ARMv7 engine')
        data=z.read(entries[0])
    if hashlib.sha256(data).hexdigest()!=ENGINE_SHA256:
        raise ValueError('Unsupported engine SHA256; do not patch around this check')
    output.parent.mkdir(parents=True,exist_ok=True)
    with output.open('xb') as f:f.write(data)
    return ENGINE_SHA256
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('apk',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    try:print(extract(a.apk,a.output))
    except (ValueError,OSError,zipfile.BadZipFile) as error:raise SystemExit(str(error))
