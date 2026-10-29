"""Synthetic mixed-generation, corruption and path-escape tests; no game input."""
import hashlib,json,struct,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from check_build_inputs import check

class InputsTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)/'resources';self.root.mkdir()
        body=bytes(304);identity=14695981039346656037
        for x in body:identity=((identity^x)*1099511628211)&((1<<64)-1)
        self.files={'direct-index.bin':struct.pack('<4IQ',0x31435053,1,1,304,identity)+body,
            'direct_cache_identity.h':f'#define DIRECT_CACHE_ID UINT64_C({identity})'.encode(),
            'textures/data.bin':b'fake pack','textures/index.bin':b'fake fallback index',
            'loading-pack.bin':struct.pack('<IIQII',0x314c5053,1,identity,0,24),
            'launcher-splashes.bin':b'SPLASH01'+bytes(2*640*360*4),
            'sce_sys/livearea/contents/template.xml':b'<livearea/>'}
        for name,w,h in [('icon0.png',128,128),('livearea/contents/bg0.png',840,500),('livearea/contents/startup.png',280,158)]:
            self.files['sce_sys/'+name]=b'\x89PNG\r\n\x1a\n'+struct.pack('>I',13)+b'IHDR'+struct.pack('>IIBBBBB',w,h,8,3,0,0,0)
        for name,data in self.files.items():
            p=self.root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
        self.mark()
    def tearDown(self):self.temp.cleanup()
    def mark(self):
        (self.root/'completed.json').write_text(json.dumps({'completed':True,'files':{n:hashlib.sha256((self.root/n).read_bytes()).hexdigest() for n in self.files}}))
    def test_accept_matching_set(self):self.assertEqual(check(self.root),1)
    def test_detect_payload_corruption(self):
        (self.root/'textures/data.bin').write_bytes(b'changed')
        with self.assertRaises(ValueError):check(self.root)
    def test_reject_mixed_loading_identity(self):
        p=self.root/'loading-pack.bin';b=bytearray(p.read_bytes());b[8]^=1;p.write_bytes(b);self.mark()
        with self.assertRaises(ValueError):check(self.root)
    def test_reject_external_symlink(self):
        outside=Path(self.temp.name)/'outside';outside.write_bytes(b'fake pack')
        p=self.root/'textures/data.bin';p.unlink();p.symlink_to(outside)
        with self.assertRaises(ValueError):check(self.root)
    def test_reject_git_parent(self):
        (Path(self.temp.name)/'.git').mkdir()
        with self.assertRaises(ValueError):check(self.root)
if __name__=='__main__':unittest.main()
