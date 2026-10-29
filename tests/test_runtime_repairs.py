import hashlib
import json
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class RuntimeRepairs(unittest.TestCase):
    def test_generated_compatibility_sources_match_reviewed_patches(self):
        changes = json.loads((ROOT/'tools/integration/patches.json').read_text())['changes']
        lock = json.loads((ROOT/'tools/integration/source-lock.json').read_text())['origin']['gitBlobs']
        for file in ('source/common.cpp', 'source/indexing.cpp', 'source/enum.cpp'):
            raw = (ROOT/'vendor/origin-upstream'/file).read_bytes()
            digest = hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()
            self.assertEqual(digest, lock[file])
            expected = raw.decode('latin1').replace('\r\n','\n')
            for change in changes:
                if change['file'] == file:
                    self.assertEqual(expected.count(change['old']), change['count'])
                    expected = expected.replace(change['old'],change['new'])
            actual = (ROOT/'vendor/origin-patched'/file).read_bytes().decode('latin1').replace('\r\n','\n')
            self.assertEqual(expected,actual)

    def test_matcher_has_no_algorithm_patch(self):
        self.assertEqual((ROOT/'vendor/origin-upstream/source/matcher.cpp').read_bytes(),
                         (ROOT/'vendor/origin-patched/source/matcher.cpp').read_bytes())

if __name__ == '__main__':
    unittest.main()
