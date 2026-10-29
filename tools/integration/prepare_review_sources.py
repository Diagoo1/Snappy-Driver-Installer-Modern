"""Prepare pinned Origin sources for portable structural review (not a native build)."""
from pathlib import Path, PurePosixPath
import hashlib, json, urllib.request, zipfile
ROOT=Path(__file__).resolve().parents[2]
def blob(data):return hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
def main():
    config=json.loads((ROOT/'tools/integration/source-lock.json').read_text('utf-8'))['origin']
    manifest=json.loads((ROOT/'tools/integration/patches.json').read_text('utf-8'))
    cache=ROOT/'.source-cache';cache.mkdir(exist_ok=True);archive=cache/('origin-'+config['commit']+'.zip')
    if not archive.exists():
        with urllib.request.urlopen(config['url'],timeout=120) as response:data=response.read(256*1024*1024+1)
        if len(data)>256*1024*1024:raise RuntimeError('Source archive exceeds review limit.')
        archive.write_bytes(data)
    required=set(config['gitBlobs'])|{c['file'] for c in manifest['changes']}
    with zipfile.ZipFile(archive) as package:
        root=package.namelist()[0].split('/')[0]+'/'+config['subdirectory']+'/'
        for relative in required:
            path=PurePosixPath(relative)
            if path.is_absolute() or '..' in path.parts:raise RuntimeError('Unexpected source path.')
            member=package.getinfo(root+relative)
            if member.file_size>16*1024*1024:raise RuntimeError('Unexpected source file size.')
            data=package.read(member)
            if relative in config['gitBlobs'] and blob(data)!=config['gitBlobs'][relative]:raise RuntimeError('Pinned source fingerprint differs: '+relative)
            target=ROOT/'.review/origin-upstream'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
            changes=[c for c in manifest['changes'] if c['file']==relative]
            if changes:
                value=data.decode('latin1').replace('\r\n','\n')
                for change in changes:
                    if value.count(change['old'])!=change['count']:raise RuntimeError('Patch precondition differs: '+relative)
                    value=value.replace(change['old'],change['new'])
                data=value.encode('latin1')
            target=ROOT/'.review/origin-patched'/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    print('Pinned review sources verified and patches applied.')
if __name__=='__main__':main()
