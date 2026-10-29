"""Generate a private index and its matching header from explicit local inputs."""
import argparse, ctypes as C, hashlib, json, struct
from pathlib import Path
from prepare_path_cache import inside, fnv

def build(manifest, assets, pack, output, header, hash64=fnv):
    manifest, assets, pack, output, header = map(Path, (manifest, assets, pack, output, header))
    if output.resolve() == header.resolve():
        raise ValueError("Index and header must be separate files")
    for destination in (output, header):
        if destination.resolve().is_relative_to(assets.resolve()) or destination.resolve() in (manifest.resolve(), pack.resolve()):
            raise ValueError("Output must not overwrite input data")
        if destination.exists():
            raise ValueError("Output exists; use new output paths")
    m = json.loads(manifest.read_text())
    if m.get('schema') != 'spire-path-cache' or m.get('version') != 1 or not 0 < len(m['entries']) <= 4096:
        raise ValueError("Unsupported manifest")
    with pack.open('rb') as f:
        if pack.stat().st_size != m['pack']['bytes'] or hashlib.file_digest(f, 'sha256').hexdigest() != m['pack']['sha256']:
            raise ValueError("Pack mismatch")
    records, names, previous = [], set(), None
    with pack.open('rb') as f:
        for e in m['entries']:
            name = e['path'].encode('utf-8')
            if not name or len(name) >= 256 or b'\0' in name or name in names:
                raise ValueError("Invalid or duplicate asset name")
            if previous is not None and name <= previous:
                raise ValueError("Manifest names must be strictly sorted")
            previous = name
            names.add(name)
            src = inside(assets, e['path']).read_bytes()
            if len(src) != e['source_bytes'] or hashlib.sha256(src).hexdigest() != e['source_sha256']:
                raise ValueError("Source mismatch")
            offset, size = e['pack_offset'], e['entry_bytes']
            if not 0 <= offset <= m['pack']['bytes'] or not 40 <= size <= m['pack']['bytes'] - offset:
                raise ValueError("Invalid pack extent")
            f.seek(offset); data = f.read(size)
            if hashlib.sha256(data).hexdigest() != e['entry_sha256']:
                raise ValueError("Entry mismatch")
            records.append(name.ljust(256, b'\0') + struct.pack('<QQ8I', hash64(src), hash64(data), len(src), offset, size, e['source_format'], *e['logical'], *e['physical']))
    body = b''.join(records); identity = hash64(body)
    raw = struct.pack('<4IQ', 0x31435053, 1, len(records), 304, identity) + body
    output.parent.mkdir(parents=True, exist_ok=True); header.parent.mkdir(parents=True, exist_ok=True)
    # All input checks precede output creation. Neither existing file is overwritten.
    with output.open('xb') as f: f.write(raw)
    with header.open('x') as f: f.write('/* Generated from verified local assets. */\n#define DIRECT_CACHE_ID UINT64_C(' + str(identity) + ')\n')
    return dict(entries=len(records), bytes=len(raw), identity=hex(identity), sha256=hashlib.sha256(raw).hexdigest())

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('manifest', 'assets', 'pack', 'output', 'header'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--hash-library', type=Path)
    a = p.parse_args(); hash64 = fnv
    if a.hash_library:
        lib = C.CDLL(str(a.hash_library.resolve()))
        lib.texture_cache_hash.argtypes = [C.c_void_p, C.c_size_t]; lib.texture_cache_hash.restype = C.c_uint64
        hash64 = lambda data: lib.texture_cache_hash(data, len(data))
    print(json.dumps(build(a.manifest, a.assets, a.pack, a.output, a.header, hash64), indent=2))
