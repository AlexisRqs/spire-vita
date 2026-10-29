"""Synthetic input validation; no game data, SDK, network or Vita required."""
import hashlib, importlib.util, json, pathlib, struct, sys, tempfile, unittest, zipfile
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from build_direct_cache_index import build
from check_build_index import validate
from extract_engine import extract, ENTRY

class ResourceTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.p=pathlib.Path(self.temp.name);self.assets=self.p/'assets';self.assets.mkdir()
        self.source=b'synthetic source';self.entry=b'X'*48
        (self.assets/'example').write_bytes(self.source);(self.p/'pack').write_bytes(self.entry)
        sha=lambda b:hashlib.sha256(b).hexdigest()
        self.manifest={'schema':'spire-path-cache','version':1,'pack':{'bytes':48,'sha256':sha(self.entry)},'entries':[{'path':'example','source_bytes':len(self.source),'source_sha256':sha(self.source),'pack_offset':0,'entry_bytes':48,'entry_sha256':sha(self.entry),'source_format':0x9274,'logical':[4,4],'physical':[4,4]}]}
    def invoke(self):
        (self.p/'manifest').write_text(json.dumps(self.manifest))
        return build(self.p/'manifest',self.assets,self.p/'pack',self.p/'index',self.p/'identity.h')
    def test_roundtrip_and_corruption(self):
        self.assertEqual(self.invoke()['entries'],1)
        self.assertEqual(validate(self.p/'index',self.p/'identity.h'),1)
        raw=(self.p/'index').read_bytes();(self.p/'index').write_bytes(raw[:-1]+bytes([raw[-1]^1]))
        with self.assertRaises(ValueError):validate(self.p/'index',self.p/'identity.h')
    def test_traversal(self):
        self.manifest['entries'][0]['path']='../outside'
        with self.assertRaises(ValueError):self.invoke()
        self.assertFalse((self.p/'index').exists())
    def test_pack_corruption(self):
        (self.p/'pack').write_bytes(b'Y'*48)
        with self.assertRaises(ValueError):self.invoke()
    def test_source_corruption(self):
        (self.assets/'example').write_bytes(b'changed')
        with self.assertRaises(ValueError):self.invoke()
    def test_duplicate(self):
        self.manifest['entries']*=2
        with self.assertRaises(ValueError):self.invoke()
    def test_extent(self):
        self.manifest['entries'][0]['pack_offset']=48
        with self.assertRaises(ValueError):self.invoke()
    def test_existing_output_preserved(self):
        (self.p/'identity.h').write_text('keep')
        with self.assertRaises(ValueError):self.invoke()
        self.assertEqual((self.p/'identity.h').read_text(),'keep')
        self.assertFalse((self.p/'index').exists())
    def test_wrong_identity(self):
        self.invoke();(self.p/'identity.h').write_text('#define DIRECT_CACHE_ID UINT64_C(0)')
        with self.assertRaises(ValueError):validate(self.p/'index',self.p/'identity.h')

    def test_missing_engine(self):
        with zipfile.ZipFile(self.p/'sample.apk','w') as z:z.writestr('other',b'none')
        with self.assertRaises(ValueError):extract(self.p/'sample.apk',self.p/'engine')
        self.assertFalse((self.p/'engine').exists())
    def test_unknown_engine(self):
        with zipfile.ZipFile(self.p/'sample.apk','w') as z:z.writestr(ENTRY,b'unsupported')
        with self.assertRaises(ValueError):extract(self.p/'sample.apk',self.p/'engine')
        self.assertFalse((self.p/'engine').exists())
    def test_unsorted_manifest(self):
        a=dict(self.manifest['entries'][0]);a['path']='zzz';(self.assets/'zzz').write_bytes(self.source)
        self.manifest['entries'].insert(0,a)
        with self.assertRaises(ValueError):self.invoke()

if __name__=='__main__':unittest.main()
