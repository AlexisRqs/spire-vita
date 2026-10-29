#!/usr/bin/env python3
"""Synthetic Git boundary tests for the publication checker (no real inputs)."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

spec=importlib.util.spec_from_file_location('publication',Path(__file__).with_name('check_publication.py'))
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

class PublicationTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.git('init','-q');self.git('config','user.name','Test');self.git('config','user.email','test@example.invalid')
    def tearDown(self):self.temp.cleanup()
    def git(self,*args):return subprocess.check_output(['git',*args],cwd=self.root,stderr=subprocess.DEVNULL)
    def add(self,name,data):
        (self.root/name).write_bytes(data);self.git('add',name)
    def test_plain_source(self):
        self.add('main.c',b'int main(void){return 0;}');self.assertTrue(audit.audit(self.root)['ok'])
    def test_reject_rom_and_symlink(self):
        self.add('game.gba',b'fake');(self.root/'link').symlink_to('main.c');self.git('add','link')
        result=audit.audit(self.root);self.assertFalse(result['ok']);self.assertEqual(len(result['issues']),2)
    def test_old_blob_survives_deletion(self):
        self.add('game.gba',b'fake');self.git('commit','-qm','private input')
        self.git('rm','game.gba');self.add('main.c',b'int x;');self.git('commit','-qm','remove input')
        self.assertTrue(audit.audit(self.root)['ok']);self.assertFalse(audit.audit(self.root,True)['ok'])
    def test_history_mode_detects_old_symlink(self):
        (self.root/'link').symlink_to('source');self.git('add','link');self.git('commit','-qm','symlink')
        self.git('rm','link');self.add('source',b'plain text');self.git('commit','-qm','remove symlink')
        self.assertFalse(audit.audit(self.root,True)['ok'])
    def test_reject_token_without_echo(self):
        fake=b'gh' + b'p_' + b'A'*30;self.add('config.txt',fake)
        result=audit.audit(self.root);self.assertFalse(result['ok']);self.assertNotIn(fake.decode(),str(result))
    def test_reject_hidden_binary(self):
        self.add('data.txt',b'header\0bytes');self.assertFalse(audit.audit(self.root)['ok'])
if __name__=='__main__':unittest.main()
